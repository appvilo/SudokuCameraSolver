//
// Created by Silverstar on 4/18/2019.
//

#ifndef SUDOKU_YOLO_H
#define SUDOKU_YOLO_H

#endif //SUDOKU_YOLO_H

#include <string>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <fstream>
#include <sstream>

using namespace cv;
using namespace std;
using namespace dnn;

class CYolo {
private:
    int inpWidth;
    int inpHeight;
    float scale;
    Net net;
    float confThreshold;
    float nmsThreshold;
    Scalar smean;
    std::vector<std::string> classes;
    std::string top_label;
    bool bDrawBox;

private:
    string postprocess(Mat& frame, const std::vector<Mat>& out, Net& net);
    void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame);
    std::vector<String> getOutputsNames(const Net& net);
public:
    CYolo();
    bool Init(string &modelFile, string &cfgFile, string &nameFile);
    string Detection(Mat &img, bool bDrawBox);
};

CYolo::CYolo() {
    inpWidth = 416;
    inpHeight = 416;
    scale = 0.00392;
    confThreshold = 0.5;
    nmsThreshold = 0.4;
    smean = Scalar();
    classes.clear();
    bDrawBox = true;
}

bool CYolo::Init(string &modelFile, string &cfgFile, string &nameFile) {
    classes.clear();
    //std::string file = "voc.names";
    std::ifstream ifs(nameFile.c_str());
    if (!ifs.is_open())
        return false;
    //CV_Error(Error::StsError, "File " + file + " not found");
    std::string line;
    while (std::getline(ifs, line))
    {
        classes.push_back(line);
    }

    net = readNet(modelFile, cfgFile, "");
    net.setPreferableBackend(0);
    net.setPreferableTarget(0);
    bDrawBox = true;
    return true;
}

string CYolo::Detection(Mat &frame, bool DrawBox) {
    bDrawBox = DrawBox;
    Mat blob;
    if (frame.empty())
    {
        return "";
    }
    Size inpSize(inpWidth > 0 ? inpWidth : frame.cols,
                 inpHeight > 0 ? inpHeight : frame.rows);
    blobFromImage(frame, blob, scale, inpSize, 0/*smean*/, true, false);

    // Run a model.
    net.setInput(blob);
    /*
    if (w_pNet->getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
    {
        resize(frame, frame, inpSize);
        Mat imInfo = (Mat_<float>(1, 3) << inpSize.height, inpSize.width, 1.6f);
        w_pNet->setInput(imInfo, "im_info");
    }*/
    std::vector<Mat> outs;
    net.forward(outs, getOutputsNames(net));

    string result = postprocess(frame, outs, net);

    // Put efficiency information.
    std::vector<double> layersTimes;
    double freq = getTickFrequency() / 1000;
    double t = net.getPerfProfile(layersTimes) / freq;
    if(bDrawBox)
    {
        std::string label = format("Inference time: %.2f ms", t);
        putText(frame, label, Point(0, 15), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0));
    }
    return result;
}

std::vector<String> CYolo::getOutputsNames(const Net& net)
{
    static std::vector<String> names;
    if (names.empty())
    {
        std::vector<int> outLayers = net.getUnconnectedOutLayers();
        std::vector<String> layersNames = net.getLayerNames();
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i)
            names[i] = layersNames[outLayers[i] - 1];
    }
    return names;
}

void CYolo::drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
    rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 255, 0));

    std::string label = format("%.2f", conf);
    if (!classes.empty())
    {
        CV_Assert(classId < (int)classes.size());
        label = classes[classId] + ": " + label;
    }

    int baseLine;
    Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    top = max(top, labelSize.height);
    rectangle(frame, Point(left, top - labelSize.height),
              Point(left + labelSize.width, top + baseLine), Scalar::all(255), FILLED);
    putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0,0,255),2);
}

string CYolo::postprocess(Mat& frame, const std::vector<Mat>& outs, Net& net)
{
    static std::vector<int> outLayers = net.getUnconnectedOutLayers();
    static std::string outLayerType = net.getLayer(outLayers[0])->type;

    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<Rect> boxes;
    if (net.getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
    {
        // Network produces output blob with a shape 1x1xNx7 where N is a number of
        // detections and an every detection is a vector of values
        // [batchId, classId, confidence, left, top, right, bottom]
        CV_Assert(outs.size() == 1);
        float* data = (float*)outs[0].data;
        for (size_t i = 0; i < outs[0].total(); i += 7)
        {
            float confidence = data[i + 2];
            if (confidence > confThreshold)
            {
                int left = (int)data[i + 3];
                int top = (int)data[i + 4];
                int right = (int)data[i + 5];
                int bottom = (int)data[i + 6];
                int width = right - left + 1;
                int height = bottom - top + 1;
                classIds.push_back((int)(data[i + 1]) - 1);  // Skip 0th background class id.
                boxes.push_back(Rect(left, top, width, height));
                confidences.push_back(confidence);
            }
        }
    }
    else if (outLayerType == "DetectionOutput")
    {
        // Network produces output blob with a shape 1x1xNx7 where N is a number of
        // detections and an every detection is a vector of values
        // [batchId, classId, confidence, left, top, right, bottom]
        CV_Assert(outs.size() == 1);
        float* data = (float*)outs[0].data;
        for (size_t i = 0; i < outs[0].total(); i += 7)
        {
            float confidence = data[i + 2];
            if (confidence > confThreshold)
            {
                int left = (int)(data[i + 3] * frame.cols);
                int top = (int)(data[i + 4] * frame.rows);
                int right = (int)(data[i + 5] * frame.cols);
                int bottom = (int)(data[i + 6] * frame.rows);
                int width = right - left + 1;
                int height = bottom - top + 1;
                classIds.push_back((int)(data[i + 1]) - 1);  // Skip 0th background class id.
                boxes.push_back(Rect(left, top, width, height));
                confidences.push_back(confidence);
            }
        }
    }
    else if (outLayerType == "Region")
    {
        for (size_t i = 0; i < outs.size(); ++i)
        {
            // Network produces output blob with a shape NxC where N is a number of
            // detected objects and C is a number of classes + 4 where the first 4
            // numbers are [center_x, center_y, width, height]
            float* data = (float*)outs[i].data;
            for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
            {
                Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
                Point classIdPoint;
                double confidence;
                minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
                if (confidence > confThreshold)
                {
                    int centerX = (int)(data[0] * frame.cols);
                    int centerY = (int)(data[1] * frame.rows);
                    int width = (int)(data[2] * frame.cols);
                    int height = (int)(data[3] * frame.rows);
                    int left = centerX - width / 2;
                    int top = centerY - height / 2;

                    classIds.push_back(classIdPoint.x);
                    confidences.push_back((float)confidence);
                    boxes.push_back(Rect(left, top, width, height));
                }
            }
        }
    }
    else
        CV_Error(Error::StsNotImplemented, "Unknown output layer type: " + outLayerType);

    std::vector<int> indices;
    NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);
    string result = "";
    for (size_t i = 0; i < indices.size(); ++i)
    {
        int idx = indices[i];
        Rect box = boxes[idx];
        int obj_id = classIds[idx];

        string temp;
        stringstream ss;

        char out_string[8];
        sprintf(out_string, "%d", obj_id);
        temp.append(out_string);
        temp.append("|");

        sprintf(out_string, "%d", box.x);
        temp.append(out_string);
        temp.append("|");

        sprintf(out_string, "%d", box.y);
        temp.append(out_string);
        temp.append("|");

        sprintf(out_string, "%d", box.width);
        temp.append(out_string);
        temp.append("|");

        sprintf(out_string, "%d", box.height);
        temp.append(out_string);

        result.append(temp);
        result.append("#");

        if(bDrawBox)
            drawPred(classIds[idx], confidences[idx], box.x, box.y, box.x + box.width, box.y + box.height, frame);
    }
//    float max = -10;
//    int max_idx = 0;
//    if (indices.size() > 0)
//    {
//        for (size_t i = 0; i < indices.size(); ++i)
//        {
//            int idx = indices[i];
//            if (max < confidences[idx])
//            {
//                max = confidences[idx];
//                max_idx = classIds[idx];
//            }
//        }
//        top_label = classes[max_idx];
//    }
//    else
//    {
//        top_label = "";
//    }
    return result;
}


