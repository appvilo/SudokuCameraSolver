//
// Created by Silverstar on 4/5/2019.
//

#ifndef SUDOKU_UTILITY_H
#define SUDOKU_UTILITY_H

#endif //OMR_SCANNER_UTILITY_H

#include <jni.h>
#include <opencv2/opencv.hpp>
#include <android/asset_manager_jni.h>
#include <android/log.h>

jint g_preW = 300, g_preH = 120;

using namespace cv;
using namespace std;

struct st_block {
    Point p1;
    Point p2;
    int row = 0;
    int col = 0;
    st_block(Point pp1, Point pp2, int row1 = 0, int col1 = 0) {
        p1 = pp1;
        p2 = pp2;
        row = row1;
        col = col1;
    };
    st_block() = default;
};

st_block block_100[4] = {{ Point(208, 706), Point(448, 2047), 25, 4},
                         { Point(559, 706), Point(797, 2047), 25, 4},
                         { Point(910, 706), Point(1148, 2047), 25, 4},
                         { Point(1261, 706), Point(1499, 2047), 25, 4}};

st_block block_180[4] = {{Point(246, 674),  Point(457, 2130),  45, 4},
                         {Point(585, 674),  Point(796, 2130),  45, 4},
                         {Point(924, 674),  Point(1135, 2130),  45, 4},
                         {Point(1263, 674), Point(1474, 2130), 45, 4}};

st_block block_200[5] = {{ Point(180, 674), Point(392, 2130), 45, 4},
                         { Point(467, 674), Point(679, 2130), 45, 4},
                         { Point(755, 674), Point(966, 2130), 45, 4},
                         { Point(1042, 674), Point(1253, 2130), 45, 4},
                         { Point(1329, 674), Point(1540, 1331), 20, 4}};

st_block block_50[4] = {{Point(185, 664),  Point(396, 1090),  13, 4},
                        {Point(470, 664),  Point(681, 1090),  13, 4},
                        {Point(755, 664),  Point(966, 1090),  13, 4},
                        {Point(1040, 664), Point(1251, 1025), 11, 4}};

struct st_header {
    int width = 1654;
    int height = 2339;
    int gap_x = 64;
    int gap_y = 64;

    struct st_block rfid = {Point(99, 295), Point(342, 628), 10, 5};
    struct st_block booklet = {Point(365, 234), Point(609, 350), 1, 4};
    struct st_block barcode = {Point(894, 416), Point(1534, 625)};
    st_header() = default;
} head;

struct omr_template {
    struct st_header header;
    int n_100 = 4;
    struct st_block* b_100;
    int n_180 = 4;
    struct st_block* b_180;
    int n_200 = 5;
    struct st_block* b_200;
    int n_50 = 4;
    struct st_block* b_50;
    omr_template(st_header h,
                 int n1, st_block b1[],
                 int n2, st_block b2[],
                 int n3, st_block b3[],
                 int n4, st_block b4[]) {
        header = h;
        n_100 = n1; b_100 = b1;
        n_180 = n2; b_180 = b2;
        n_200 = n3; b_200 = b3;
        n_50 = n4; b_50 = b4;
    }
} g_omr = {head, 4, block_100, 4, block_180, 5, block_200, 4, block_50};

Point get_corner(Mat& m_mat, Rect rect, int type) {
    Mat mat;
    m_mat(rect).copyTo(mat);

    SimpleBlobDetector::Params params;

// Change thresholds
    params.minThreshold = 10;
    params.maxThreshold = 200;

// Filter by Color.
    params.filterByColor = true;
    params.blobColor  = 0;

// Filter by Area.
    params.filterByArea = true;
    float s = (m_mat.cols*50/1000)*(m_mat.cols*50/1000);
    params.maxArea = (int)s;
    params.minArea = (int)(s * 0.1);

// Filter by Circularity
    params.filterByCircularity = true;
    params.minCircularity = 0.1;

// Filter by Convexity
    params.filterByConvexity = true;
    params.minConvexity = 0.5;

// Filter by Inertia
    params.filterByInertia = true;
    params.minInertiaRatio = 0.3;

    Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);
    std::vector<KeyPoint> keypoints;
    std::vector<Point> corner;
    std::vector<float> dist;
// detection of blob in rect and get one point with shortest distance from corner.
    Point ret;
    detector->detect(mat, keypoints);
    double min = 1000000;
    for (int i=0; i<keypoints.size(); i++){
        Point pt = keypoints[i].pt;

        double d;
        if(type == 0)
            d = sqrt((rect.x-pt.x)*(rect.x-pt.x)+(rect.y-pt.y)*(rect.y-pt.y));
        else if(type == 1)
            d = sqrt((rect.x+rect.width - pt.x)*(rect.x+rect.width - pt.x)+(rect.y-pt.y)*(rect.y-pt.y));
        else if(type == 2)
            d = sqrt((rect.x-pt.x)*(rect.x-pt.x)+(rect.y+rect.height - pt.y)*(rect.y+rect.height - pt.y));
        else
            d = sqrt((rect.x+rect.width - pt.x)*(rect.x+rect.width - pt.x)+(rect.y+rect.height-pt.y)*(rect.y+rect.height - pt.y));

        if(min > d) {
            min = d;
            ret = Point(rect.x + pt.x, rect.y + pt.y);
        }
    }
    return ret;
}

void DrawBlock(Mat& inputImg) {
    Rect rect_rfid, rect_booklet, rect_barcode;
    //----------------Get rfid rect
    rect_rfid.x = g_omr.header.rfid.p1.x - g_omr.header.gap_x;
    rect_rfid.y = g_omr.header.rfid.p1.y - g_omr.header.gap_y;
    rect_rfid.width = g_omr.header.rfid.p2.x - g_omr.header.rfid.p1.x;
    rect_rfid.height = g_omr.header.rfid.p2.y - g_omr.header.rfid.p1.y;

    //----------------Get booklet rect
    rect_booklet.x = g_omr.header.booklet.p1.x - g_omr.header.gap_x + 20;
    rect_booklet.y = g_omr.header.booklet.p1.y - g_omr.header.gap_y;
    rect_booklet.width = g_omr.header.booklet.p2.x - g_omr.header.booklet.p1.x - 40;
    rect_booklet.height = g_omr.header.booklet.p2.y - g_omr.header.booklet.p1.y;

    //----------------Get barcode rect
    int gap = 20;
    rect_barcode.x = g_omr.header.barcode.p1.x - g_omr.header.gap_x - gap;
    rect_barcode.y = g_omr.header.barcode.p1.y - g_omr.header.gap_y - gap;
    rect_barcode.width = g_omr.header.barcode.p2.x - g_omr.header.barcode.p1.x + 2*gap;
    rect_barcode.height = g_omr.header.barcode.p2.y - g_omr.header.barcode.p1.y + 2*gap;
    rectangle(inputImg, rect_rfid, Scalar(0, 255, 0), 5);
    rectangle(inputImg, rect_booklet, Scalar(0, 255, 0), 5);
    rectangle(inputImg, rect_barcode, Scalar(0, 255, 0), 5);
}

/**
 * input: Gray
 * output: bool
 */
bool HaveDetectRightLine(Mat& mat, Mat& out) {
    Mat binary, temp;
    Rect rect;
    rect.x = mat.cols * 5 / 6;
    rect.y = 0;
    rect.width = mat.cols / 6;
    rect.height = mat.rows;

    mat(rect).copyTo(temp);

    cvtColor(temp, temp, COLOR_BGRA2GRAY);
    normalize(temp, binary, 0, 255, NORM_MINMAX, CV_8UC1);

    int erosion_type = MORPH_RECT;
    int erosion_size = 5;
    Mat ele = getStructuringElement(erosion_type,
                                    Size(2 * erosion_size + 1, 2 * erosion_size + 1),
                                    Point(erosion_size, erosion_size));
    dilate(binary, temp, ele, Point(-1, -1), 2);

    bool ret = false;
    Canny(temp, binary, 50, 100);

    std::vector<std::vector<Point> > contours;
    std::vector<Vec4i> hierarchy;
    std::vector<Point> approxCurve;
    findContours(binary, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

    std::vector<std::vector<Point> > contours_poly(contours.size());

    for (int i = 0; i < contours.size(); i++)
    {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 0.1, true);
        int w = boundingRect(Mat(contours[i])).width;
        int h = boundingRect(Mat(contours[i])).height;
        double ar = (double)h / w, ar1 = (double)temp.rows / temp.cols;
        if (h > temp.rows*0.5 && w < temp.cols*0.5) {
            //rectangle(out, boundingRect(Mat(contours[i])), Scalar(0, 0, 255), 1);
            ret = true;
            break;
        }
    }
    return ret;
}


string getAnswers(int *value, int row, int col) {
    string result("");
    char label[4] = {'A', 'B', 'C', 'D'};
    for(int i=0;i<row;i++) {
        string sr("");
        for(int j=0;j<col;j++) {
            int answer = *(value + j + i*col);
            if(answer == 1) {
                char an_label = label[j];
                sr.append(1, an_label);
                sr.append(1, '#');
            }
        }
        if(sr.empty()) {
            result.append("?,");
        } else {
            sr.pop_back();
            result.append(sr);
            result.append(1, ',');
        }
    }
    return result;
}

string getRfid(int *value, int row, int col) {
    string result("");
    char label[10] = {'0', '1', '2', '3','4', '5', '6', '7','8', '9'};
    for(int j=0;j<col;j++) {
        string sr("");
        for(int i=0;i<row;i++) {
            int answer = *(value + j + i*col);
            if(answer == 1) {
                char an_label = label[i];
                sr.append(1, an_label);
                sr.append(1, '#');
            }
        }
        if(sr.empty()) {
            result.append("?");
        } else {
            sr.pop_back();
            result.append(sr);
//            result.append(1, ' ');
        }
    }
    return result;
}

string getBooklet(int *value, int col) {
    string result("");
    char label[4] = {'1', '2', '3', '4'};
    for(int j=0;j<col;j++) {
        int answer = *(value + j);
        if (answer == 1) {
            char an_label = label[j];
            result.append(1, an_label);
            result.append(1, an_label);
            result.append(1, '#');
        }
    }
    if(result.empty()) {
        result.append("11");
    } else {
        result.pop_back();
        std::size_t found = result.find("#");
        if (found!=std::string::npos)
            result = "11";
    }
    return result;
}

int* recog_oneblock(Mat& in_img, Rect rcOrg, int nRows, int nCols, Mat& outImg) {
    Mat matTmp, binary_img;
    int* res_matrix = (int*)malloc(nRows*nCols * sizeof(int));
    memset(res_matrix, 0, nRows*nCols * sizeof(int));
    Rect inerRc(rcOrg.x + 15, rcOrg.y + 10, rcOrg.width - 30, rcOrg.height - 20);
    rectangle(outImg, inerRc, Scalar(0, 0, 255), 2);
    in_img(inerRc).copyTo(matTmp);

    GaussianBlur(matTmp, binary_img, Size(11, 11), 10);
    cvtColor(binary_img, matTmp, COLOR_BGRA2GRAY);
    normalize(matTmp, binary_img, 0, 255, NORM_MINMAX, CV_8UC1);

    int erosion_type = MORPH_RECT;
    int erosion_size = 1;
    Mat ele = getStructuringElement(erosion_type,
                                    Size(2 * erosion_size + 1, 2 * erosion_size + 1),
                                    Point(erosion_size, erosion_size));
    dilate(binary_img, matTmp, ele, Point(-1, -1), 1);

    threshold(binary_img, matTmp, 90, 255, THRESH_BINARY);
    bool ret = false;
    Canny(matTmp, binary_img, 50, 100);

    std::vector<std::vector<Point> > contours;
    std::vector<Vec4i> hierarchy;
    //Find document countour
    //binary_img.copyTo(outImg);
    findContours(binary_img, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

    std::vector<std::vector<Point>> contours_poly(contours.size());
    std::vector<Point> mc;
    for (int i = 0; i < contours.size(); i++)
    {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 0.1, true);
        int w = boundingRect(Mat(contours[i])).width;
        int h = boundingRect(Mat(contours[i])).height;
        double ar = (double)w / h;

        if (hierarchy[i][3] == -1) //No parent
            if (w >= 10 && h >= 10) {
                Moments mu = moments(contours_poly[i], false);
                Point center = Point(mu.m10 / mu.m00, mu.m01 / mu.m00);
                mc.push_back(center);
            }
    }

    float dh = inerRc.height / nRows;
    for (int i = 0; i < nRows; i++) {
        std::vector<Point> mc_array; //one line
        for (int j = 0; j < mc.size(); j++) {
            if (mc[j].y > (int)(i*dh) && mc[j].y < (int)((i + 1)*dh)) {
                mc_array.push_back(mc[j]);
                //circle(outImg, Point(mc[j].x + inerRc.x, mc[j].y+ inerRc.y), 15, Scalar(255, 0, 0), 2);
            }
        }

        int* row_mark = (int*)malloc(nCols * sizeof(int));
        memset(row_mark, 0, nCols * sizeof(int));

        float gap = inerRc.width / (float)nCols;
        for (int k = 0; k < nCols; k++) {
            for (int j = 0; j < mc_array.size(); j++) {
                if (mc_array[j].x >= k * gap && mc_array[j].x < (k + 1)*gap) {
                    row_mark[k] = 1;
                    break;
                }
                circle(outImg, Point(mc_array[j].x + inerRc.x, mc_array[j].y + inerRc.y), 15, Scalar(255, 0, 0), 2);
            }
        }

        for (int jj = 0; jj < nCols; jj++)
            res_matrix[i*nCols + jj] = row_mark[jj];
        free(row_mark);
    }

    return res_matrix;
}