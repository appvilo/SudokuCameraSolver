#include <jni.h>
#include <string>
#include <opencv2/opencv.hpp>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#include "utility.h"
#include "yolo.h"

using namespace cv;
using namespace std;

vector<Point2f> g_finalRect;
CYolo *w_pNet=NULL;
float g_alpha = 0.4;

vector<Point2f> pre_processing(Mat& input, Mat& output) {
    Mat img_gray;

    cvtColor(input, img_gray, COLOR_RGBA2GRAY);

    int nH = input.rows, nW = input.cols;
    double nThres = nH * g_alpha;

    GaussianBlur(img_gray, img_gray, Size(11, 11), 0);
    adaptiveThreshold(img_gray, img_gray, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 5, 2);
    bitwise_not(img_gray, img_gray);

    cv::Mat element(3, 3, CV_8U, cv::Scalar(1));
    dilate(img_gray, img_gray, element, Point(-1, -1), 1);

//    img_gray.copyTo(output);

    vector<vector<Point> > contours;
    findContours(img_gray, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

    vector<Point2f> approx, finalRect, ret;

    double nMin = nH * nW;
    Point2f center = Point2f(input.cols/2, input.rows/2);
    for(size_t i = 0; i < contours.size(); i++)
    {
        approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.1, true);

        Rect rect = boundingRect(Mat(contours[i]));
        int w = rect.width;
        int h = rect.height;

        if (approx.size()==4 && w>= nThres && h>= nThres && isContourConvex(Mat(approx)))
        {
//            if (fabs(contourArea(Mat(approx)) < nMin)) {
            double in = pointPolygonTest(approx, center, false);
            if(w*h<nMin && in > 0){
                nMin = fabs(contourArea(Mat(approx)));
                finalRect = approx;

                Moments m = moments(approx, true);
                center = Point2f(m.m10/m.m00, m.m01/m.m00);
            }
        }
    }

//    if(!finalRect.empty()) {
//        Point2f lt, lb, rt, rb;
//        for (int i = 0; i < 4; i++) {
//            Point p = finalRect[i];
//            double dx = p.x - center.x;
//            double dy = p.y - center.y;
//
//            if (dx < 0 && dy < 0) {
//                lt = Point2f(p.x, p.y);
//            } else if (dx > 0 && 0 > dy) {
//                rt = Point2f(p.x, p.y);
//            } else if (dx > 0 && dy > 0) {
//                rb = Point2f(p.x, p.y);
//            } else if (dx < 0 && 0 < dy) {
//                lb = Point2f(p.x, p.y);
//            }
//        }
//
//        ret.push_back(lt);
//        ret.push_back(lb);
//        ret.push_back(rt);
//        ret.push_back(rb);
//    }
    return finalRect;
}

void DrawPreRect(Mat& mat) {
    float nRH = (float)(mat.rows*0.6), gap = mat.rows/8;
    int thick = 8;

    Point2f pt1, pt2, pt3;
    pt1.x = mat.cols/2 - nRH/2, pt1.y = mat.rows/2 - nRH/2;
    pt2 = pt1, pt3 = pt1, pt2.x += gap, pt3.y += gap;

    line(mat, pt1, pt2, Scalar(0, 255, 0), thick);
    line(mat, pt1, pt3, Scalar(0, 255, 0), thick);

    pt1.x = mat.cols/2 + nRH/2, pt1.y = mat.rows/2 - nRH/2;
    pt2 = pt1, pt3 = pt1, pt2.y += gap, pt3.x -= gap;

    line(mat, pt1, pt2, Scalar(0, 255, 0), thick);
    line(mat, pt1, pt3, Scalar(0, 255, 0), thick);

    pt1.x = mat.cols/2 + nRH/2, pt1.y = mat.rows/2 + nRH/2;
    pt2 = pt1, pt3 = pt1, pt2.y -= gap, pt3.x -= gap;

    line(mat, pt1, pt2, Scalar(0, 255, 0), thick);
    line(mat, pt1, pt3, Scalar(0, 255, 0), thick);

    pt1.x = mat.cols/2 - nRH/2, pt1.y = mat.rows/2 + nRH/2;
    pt2 = pt1, pt3 = pt1, pt2.y -= gap, pt3.x += gap;

    line(mat, pt1, pt2, Scalar(0, 255, 0), thick);
    line(mat, pt1, pt3, Scalar(0, 255, 0), thick);
}


bool compareContourAreas(cv::Point2f pt1, cv::Point2f pt2) {
    return (pt1.x < pt2.x);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_gumballi_jay_sudokusolver_MainActivity_SudokuResult(JNIEnv *env, jobject instance,
                                                             jlong inputImage) {

    Mat &img_input = *(Mat *)inputImage;
    Mat rotateMat;
    string returnValue("");

    rotate(img_input, rotateMat, cv::ROTATE_90_CLOCKWISE);
    cvtColor(rotateMat, rotateMat, COLOR_BGRA2RGB);
    returnValue = w_pNet->Detection(rotateMat, true);

    return env->NewStringUTF(returnValue.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gumballi_jay_sudokusolver_MainActivity_init(JNIEnv *env, jobject instance,
                                                     jstring modelPath_, jstring cfgPath_,
                                                     jstring namePath_) {
    const char *modelPath = env->GetStringUTFChars(modelPath_, 0);
    string modelFileName(modelPath);
    const char *cfgPath = env->GetStringUTFChars(cfgPath_, 0);
    string cfgFileName(cfgPath);
    const char *namePath = env->GetStringUTFChars(namePath_, 0);
    string nameFileName(namePath);

    if(w_pNet != NULL)
        delete w_pNet;

    w_pNet = new CYolo();
    w_pNet->Init(modelFileName, cfgFileName, nameFileName);

    env->ReleaseStringUTFChars(modelPath_, modelPath);
    env->ReleaseStringUTFChars(cfgPath_, cfgPath);
    env->ReleaseStringUTFChars(namePath_, namePath);
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_gumballi_jay_sudokusolver_CameraActivity_FindCorner(JNIEnv *env, jobject instance,
                                                             jlong matAddrInput,
                                                             jlong matAddrResult,
                                                             jlong matAddrTarget) {

    Mat &img_input = *(Mat *)matAddrInput;
    Mat &img_result = *(Mat *)matAddrResult;
    Mat &img_target = *(Mat *)matAddrTarget;

    img_input.copyTo(img_result);

    /*** Detect Sudoku ***/
    vector<Point2f> finalRect;
    finalRect = pre_processing(img_input, img_result);

    /*** Draw Green Fixed Line ***/
//    DrawPreRect(img_result);

    // Draw Red Line
    int nResult = 0;
    if(!finalRect.empty() && finalRect.size() == 4) {
        int size = static_cast<int>(finalRect.size());
//        if (size == 4 && isContourConvex(Mat(finalRect)))
        { // If rectangle.
            /*** Draw Red Line ***/
            line(img_result, finalRect[0], finalRect[finalRect.size() - 1], Scalar(255, 0, 0), 10);
            for (int k = 0; k < size - 1; k++)
                line(img_result, finalRect[k], finalRect[k + 1], Scalar(255, 0, 0), 8);
            for (int k = 0; k < size; k++)
                circle(img_result, finalRect[k], 15, Scalar(0, 255, 0), 2);

            /*** Get Target Mat ***/
//            img_input.copyTo(img_result);
//            DrawPreRect(img_result);

            std::sort(finalRect.begin(), finalRect.end(), compareContourAreas);
            Point2f lt = finalRect[0], lb = finalRect[1], rt = finalRect[2], rb = finalRect[3];
            if (lt.y > lb.y) {
                lt = finalRect[1], lb = finalRect[0];
            }

            if (rt.y > rb.y) {
                rt = finalRect[3], rb = finalRect[2];
            }




            // Warp
            Mat tempMat;
            resize(img_input, tempMat, Size(540, 540));

            Point2f inputQuad[4], outputQuad[4];
            int gap = 5;
            inputQuad[0] = Point2f(lt.x+gap, lt.y+gap);
            inputQuad[1] = Point2f(rt.x-gap, rt.y+gap);
            inputQuad[2] = Point2f(rb.x-gap, rb.y-gap);
            inputQuad[3] = Point2f(lb.x+gap, lb.y-gap);

            outputQuad[0] = Point2f(0, 0);
            outputQuad[1] = Point2f(tempMat.cols - 1, 0);
            outputQuad[2] = Point2f(tempMat.cols - 1, tempMat.rows - 1);
            outputQuad[3] = Point2f(0, tempMat.rows - 1);

            Mat lambda(2, 4, CV_32FC1);
            lambda = getPerspectiveTransform(inputQuad, outputQuad);
            warpPerspective(img_input, tempMat, lambda, tempMat.size());
            tempMat.copyTo(img_target);

            nResult = 4;
        }
    }

    return nResult;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gumballi_jay_sudokusolver_CameraActivity_DrawPreRect(JNIEnv *env, jobject instance,
                                                              jlong matAddrInput) {

    Mat &img_input = *(Mat *)matAddrInput;
    DrawPreRect(img_input);
}