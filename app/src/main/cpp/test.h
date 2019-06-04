//
// Created by Silverstar on 4/5/2019.
//

#ifndef SUDOKU_TEST_H
#define SUDOKU_TEST_H

#endif //OMR_SCANNER_TEST_H

#include <jni.h>
#include <opencv2/opencv.hpp>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#include "transform.h"

using namespace cv;
using namespace std;
#define getValue(D, w, r, c) (D[(r)*(w)+(c)])

vector<Point> get_blob(Mat& m_mat, Rect rect) {
    Mat mat, temp;
    m_mat(rect).copyTo(mat);

    threshold( mat, mat, 0, 255, THRESH_BINARY_INV|THRESH_OTSU);

    Mat element(7, 7, CV_8U, Scalar(1));

    morphologyEx(mat, mat, MORPH_OPEN, element);
    morphologyEx(mat, mat, MORPH_CLOSE, element);
    bitwise_not(mat, mat);

    SimpleBlobDetector::Params params;

//// Change thresholds
    params.minThreshold = 50;
    params.maxThreshold = 220;
//
// Filter by Color.
//    params.filterByColor = true;
//    params.blobColor  = 1;
//
//// Filter by Area.
    params.filterByArea = true;
    params.minArea = 100;
    params.maxArea = 2000;
//
//// Filter by Circularity
    params.filterByCircularity = false;
    params.minCircularity = 0.3;
//
//// Filter by Convexity
    params.filterByConvexity = false;
    params.minConvexity = 0.5;
//
//// Filter by Inertia
    params.filterByInertia = false;
    params.minInertiaRatio = 0.3;

    Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);
    std::vector<KeyPoint> keypoints;

    detector->detect(mat, keypoints);

    vector<Point> ret;
    for(int i=0; i<keypoints.size(); i++){
        Point pt = keypoints[i].pt;
        ret.push_back(Point(rect.x + pt.x, rect.y + pt.y));
    }
    return ret;
}

/**
 * INPUT: in_img: Gray, outImg: RGB
 **/
int* recog_row(Mat& inputImg, Rect rcOrg, int nCols, Mat& outImg) {
    int* res_array = (int*)malloc(nCols * sizeof(int));
    memset(res_array, 0, nCols * sizeof(int));

    Mat rowImg, thres, temp;
    inputImg(rcOrg).copyTo(rowImg);

    std::vector<std::vector<Point> > contours;
    std::vector<Vec4i> hierarchy;
    std::vector<Point> approxCurve, docCnt;

    threshold( rowImg, thres, 127, 255, THRESH_BINARY_INV|THRESH_OTSU);
    Mat element(7, 7, CV_8U, Scalar(1));

    morphologyEx(thres, rowImg, MORPH_OPEN, element);
    morphologyEx(rowImg, thres, MORPH_CLOSE, element);
//    bitwise_not(mat, mat);

    findContours(thres, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

    std::vector<std::vector<Point> > contours_poly(contours.size());
    std::vector<Rect> boundRect(contours.size());
    std::vector<std::vector<Point>> questionCnt;

    for( int i = 0; i< contours.size(); i++ )
    {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 0.1, true);
        int w =  boundingRect(Mat(contours[i])).width;
        int h =  boundingRect(Mat(contours[i])).height;
        double ar = (double)w/h;

        if(hierarchy[i][3] == -1) //No parent
            if(w >=10 && h >=10 /* && ar < 1.2 && ar > 0.8 */)
                questionCnt.push_back(contours_poly[i]);
    }


//    char str[200];
//    sprintf(str,"t - %ld", questionCnt.size());
//    __android_log_write(ANDROID_LOG_DEBUG, "test", str);

    cvtColor(thres, rowImg, COLOR_GRAY2RGB);
    vector<Point> ret;
    for(int i=0; i<questionCnt.size(); i++){
        Rect r = boundingRect(Mat(questionCnt[i]));
        int cx = r.x + r.width/2, cy = r.y + r.height/2;
        int cx1 = rcOrg.x + r.x + r.width/2, cy1 = rcOrg.y + r.y + r.height/2;

        float gap = rcOrg.width/(float)nCols;

        for(int k=0;k<nCols;k++) {
            if(cx>=k*gap && cx<(k+1)*gap)
                res_array[k] = 1;
        }

        circle(outImg, Point(cx1, cy1), 15, Scalar(255,0,0), 2);
    }

//    rowImg.copyTo(outImg(rcOrg));
    return res_array;
}
/*
int* recog_row(Mat& inputImg, Rect rcOrg, int nCols, Mat& outImg)
{
   int* res_array = (int*)malloc(nCols * sizeof(int));
   memset(res_array, 0, nCols * sizeof(int));

   Mat rowImg, thres, temp;
   inputImg(rcOrg).copyTo(rowImg);
//    rectangle(outImg, rcOrg, Scalar(255, 0, 0), 1);

//    threshold(rowImg, thres, 0, 255, THRESH_BINARY|THRESH_OTSU);

   Ptr<cv::CLAHE> clahe = cv::createCLAHE();
   clahe->setClipLimit(4);
   clahe->apply(rowImg, rowImg);

   threshold(rowImg, rowImg, 0, 255, THRESH_BINARY|THRESH_OTSU);
//    adaptiveThreshold(rowImg, temp, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 7, 2);
//
//    int erosion_type = MORPH_RECT;
//    int erosion_size = 2;
//    Mat ele = getStructuringElement(erosion_type,
//                                    Size(2 * erosion_size + 1, 2 * erosion_size + 1),
//                                    Point(erosion_size, erosion_size));
//
//    erode(temp, rowImg, ele);

   int wwid = rowImg.cols, hhei = rowImg.rows;
   int* row_hist = (int*)malloc(wwid * sizeof(int));
   int y, x;

   // histogram
   for (x = 0; x < wwid; x++) {
       row_hist[x] = 0;
       for (y = 0; y < hhei; y++) {
           if (getValue(rowImg.data, wwid, y, x) == 0)
               row_hist[x] += 1;
       }
   }

   // separate each cols
   for (x = 0; x < wwid; x++) {
       if (row_hist[x] > 0) row_hist[x] = 1;
       else row_hist[x] = 0;
   }

   // hole pad
   for (x = 1; x < wwid - 1; x++) {
       if (row_hist[x] > 0) continue;
       if (row_hist[x - 1] > 0 && row_hist[x + 1] > 0)
           row_hist[x] = 1;
   }

   row_hist[wwid - 1] = 0;
   vector<int> col_posit;
   for (x = 0; x < wwid - 1; x++) {
       row_hist[x] = (row_hist[x] == row_hist[x + 1]) ? 0 : 1;
       if (row_hist[x]>0)
           col_posit.push_back(x);
   }

   int nNums = (int)col_posit.size();

   char str[200];
   sprintf(str,"%ld", col_posit.size());
   __android_log_write(ANDROID_LOG_DEBUG, "test", str);

   int sub_num = 0;
   for (x = 0; x < nNums / 2; x++) {
       Rect rc = Rect(Point(rcOrg.x + col_posit[2 * x], rcOrg.y + 0), Point(rcOrg.x + col_posit[2 * x + 1], rcOrg.y + hhei));
       Rect rc1 = Rect(Point(rcOrg.x + col_posit[2 * x], rcOrg.y + 0), Point(rcOrg.x + col_posit[2 * x + 1], rcOrg.y + hhei));
       if (nNums > 2*nCols && rc.width < 20) continue;

       rectangle(rowImg, rc, Scalar(255, 0, 0), 1);
//        // marks
//        int sums = 0;
//        for (int xx = col_posit[2 * x]; xx <= col_posit[2 * x + 1]; xx++) {
//            for (int yy = 0; yy <hhei; yy++) {
//                if (getValue(rowImg.data, wwid, yy, xx) == 0)
//                    sums += 1;
//            }
//        }
//        float thv = (float)sums / (float)rc.area();
//        if (thv > 0.3) {
//            res_array[sub_num++] = 1;
//            circle(outImg, Point(rcOrg.x + rc.x + rc.width / 2, rcOrg.y + rc.y + rc.height / 2), 3, Scalar(255, 0, 255), 3);
//        }
   }

   cvtColor(rowImg, rowImg, COLOR_GRAY2RGB);
   rowImg.copyTo(outImg(rcOrg));
   return res_array;
}
*/
/**
 * INPUT: in_img: Gray, outImg: RGB
 **/
int* recog_oneblock(Mat& in_img, Rect rcOrg, int nRows, int nCols, Mat& outImg)
{
    int* res_matrix = (int*)malloc(nRows*nCols * sizeof(int));
    memset(res_matrix, 0, nRows*nCols * sizeof(int));

    Mat tmp, binary_img;

    Rect inerRc(rcOrg.x + 10, rcOrg.y + 10, rcOrg.width - 20, rcOrg.height - 20);
    in_img(inerRc).copyTo(tmp);

    Mat kernel(3, 3, CV_32F, cv::Scalar(255));
    kernel.at<float>(1, 1) = 5.0f;
    kernel.at<float>(0, 1) = -1.0f;
    kernel.at<float>(2, 1) = -1.0f;
    kernel.at<float>(1, 0) = -1.0f;
    kernel.at<float>(1, 2) = -1.0f;

    tmp.convertTo(binary_img, -1, 0.5, 0);

//    adaptiveThreshold(tmp, binary_img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 21, 10);
//    blur(binary_img,binary_img, Size(5,5));
//    medianBlur(binary_img, binary_img, 1);
//    GaussianBlur(tmp, binary_img, Size(5,5), 0);
//    Canny(tmp, binary_img, 50, 200);
//    threshold(tmp, binary_img, 100, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

//    int erosion_type = MORPH_ELLIPSE;
//    int erosion_size = 2;
//    Mat ele = getStructuringElement(erosion_type,
//                                        Size(2 * erosion_size + 1, 2 * erosion_size + 1),
//                                        Point(erosion_size, erosion_size));
//    morphologyEx(binary_img, binary_img, MORPH_OPEN, ele, Point(-1,-1), 2);
//
    cvtColor(binary_img, binary_img, COLOR_GRAY2RGB);
    binary_img.copyTo(outImg(inerRc));

    int nWid = binary_img.cols;
    int nHei = binary_img.rows;
    int i, j;
    size_t nMemSize = nHei * sizeof(int);
    int* hist = (int*)malloc(nMemSize);

    // calculate histogram according horizontal direct
    for (i = 0; i < nHei; i++) {
        hist[i] = 0;
        for (j = 0; j < nWid; j++) {
            if (getValue(binary_img.data, nWid, i, j) == 0)
                hist[i] += 1;
        }
    }

    // separate each rows
    int t = 15;
    for (i = 0; i < nHei; i++) {
        if (hist[i] > t) hist[i] = 1;
        else hist[i] = 0;
    }
    hist[0] = 0;
    hist[nHei - 1] = 0;
    vector<int> posit;
    for (i = 0; i < nHei - 1; i++) {
        hist[i] = (hist[i] == hist[i + 1]) ? 0 : 1;
        if (hist[i]>0)
            posit.push_back(i);
    }

    char str[200];
    sprintf(str,"%ld rows detect!", posit.size()/2);
    __android_log_write(ANDROID_LOG_DEBUG, "test", str);

    int gap = 2;
    for (i = 0; i < posit.size() / 2; i++) {
        Rect roi1 = Rect(Point(inerRc.x+gap, inerRc.y + posit[2 * i] - gap), Point(inerRc.x + nWid-gap, inerRc.y + posit[2 * i + 1] + gap)); //Global

        sprintf(str,"y:%d - height: %d !", (int)roi1.y, roi1.height);

        __android_log_write(ANDROID_LOG_DEBUG, "test", str);

        int* row_mark = recog_row(in_img, roi1, nCols, outImg);
        for (int jj = 0; jj<nCols; jj++)
            res_matrix[i*nCols+jj] = row_mark[jj];
        free(row_mark);
    }
    free(hist);

    return res_matrix;
}

/*
extern "C"
JNIEXPORT void JNICALL
Java_com_silverstar_omr_1scanner_ImageActivity_preProcessing(JNIEnv *env, jobject instance,
                                                            jlong inputImage, jlong outputImage) {

    Mat &matInput = *(Mat *)inputImage;
    Mat &matResult = *(Mat *)outputImage;
    Mat temp;
    matResult.copyTo(temp);
//----------------RGB to GRAY
    cvtColor(matInput, matResult, COLOR_RGBA2GRAY);

    int preW = 1000, preH = 1000;
    Rect r_lt, r_rt, r_lb, r_rb;

//----------------Corner Rectangle
    r_lt = Rect(0, 0, preW, preH);
    r_rt = Rect(matInput.cols-preW, 0, preW, preH);
    r_lb = Rect(0,matInput.rows-preH, preW, preH);
    r_rb = Rect(matInput.cols-preW, matInput.rows-preH, preW, preH);

//----------------Get Corner Point
    Point pt_lt, pt_rt, pt_lb, pt_rb;
    pt_lt = get_corner(matResult, r_lt, 0);
    pt_rt = get_corner(matResult, r_rt, 1);
    pt_lb = get_corner(matResult, r_lb, 2);
    pt_rb = get_corner(matResult, r_rb, 3);

//    circle(matResult, Point(pt_lt.x, pt_lt.y), 50, Scalar(255,0,0), 5);
//    circle(matResult, Point(pt_rt.x, pt_rt.y), 50, Scalar(255,0,0), 5);
//    circle(matResult, Point(pt_lb.x, pt_lb.y), 50, Scalar(255,0,0), 5);
//    circle(matResult, Point(pt_rb.x, pt_rb.y), 50, Scalar(255,0,0), 5);

//    vector<Point> corner;
//    corner.push_back(pt_lt);
//    corner.push_back(pt_rt);
//    corner.push_back(pt_rb);
//    corner.push_back(pt_lb);

//----------------Perspective Transform (Crop and Resize same as Template)
    Point2f inputQuad[4], outputQuad[4];
    inputQuad[0] = pt_lt;
    inputQuad[1] = pt_rt;
    inputQuad[2] = pt_rb;
    inputQuad[3] = pt_lb;

    outputQuad[0] = Point2f( 0, 0 );
    outputQuad[1] = Point2f( matResult.cols-1, 0);
    outputQuad[2] = Point2f( matResult.cols-1, matResult.rows-1);
    outputQuad[3] = Point2f( 0, matResult.rows-1  );

    Mat lambda( 2, 4, CV_32FC1 );
    lambda = getPerspectiveTransform( inputQuad, outputQuad );
//    const Point *pts = (const cv::Point*) Mat(corner).data;
//    int  npts = Mat(corner).rows;
//    polylines(matResult, &pts, &npts, 1, true, Scalar(255, 0, 0), 5);
    warpPerspective(matResult, matResult, lambda, matResult.size() );
    warpPerspective(matInput, matInput, lambda, matInput.size() );

    Size rsize = Size(g_omr.header.width - g_omr.header.gap_x * 2, g_omr.header.height - g_omr.header.gap_y * 2);
    resize(matResult, matResult, rsize);
    resize(matInput, matInput, rsize);

//----------------Barcode Rect
    Rect barcode, rfid, booklet;
    barcode.x = g_omr.header.barcode.p1.x - g_omr.header.gap_x;
    barcode.y = g_omr.header.barcode.p1.y - g_omr.header.gap_y;
    barcode.width = g_omr.header.barcode.p2.x - g_omr.header.barcode.p1.x;
    barcode.height = g_omr.header.barcode.p2.y - g_omr.header.barcode.p1.y;

//----------------Get Rfid Rect and detection
    rfid.x = g_omr.header.rfid.p1.x - g_omr.header.gap_x;
    rfid.y = g_omr.header.rfid.p1.y - g_omr.header.gap_y;
    rfid.width = g_omr.header.rfid.p2.x - g_omr.header.rfid.p1.x;
    rfid.height = g_omr.header.rfid.p2.y - g_omr.header.rfid.p1.y;

    vector<Point> blob;
    blob = get_blob(matResult, rfid);
    for(int i=0; i<blob.size(); i++){
        Point pt = blob[i];
        circle(matInput, Point(pt.x, pt.y), 10, Scalar(255,0,0), 5);
    }

    int row = g_omr.header.rfid.row, col = g_omr.header.rfid.col;


//----------------Get booklet Rect and detection
    booklet.x = g_omr.header.booklet.p1.x - g_omr.header.gap_x;
    booklet.y = g_omr.header.booklet.p1.y - g_omr.header.gap_y;
    booklet.width = g_omr.header.booklet.p2.x - g_omr.header.booklet.p1.x;
    booklet.height = g_omr.header.booklet.p2.y - g_omr.header.booklet.p1.y;

    blob.clear();
    blob = get_blob(matResult, booklet);
    for(int i=0; i<blob.size(); i++){
        Point pt = blob[i];
        circle(matInput, Point(pt.x, pt.y), 10, Scalar(255,0,0), 5);
    }

    rectangle(matInput, barcode, Scalar(0, 255, 0), 5);
    rectangle(matInput, rfid, Scalar(0, 255, 0), 5);
    rectangle(matInput, booklet, Scalar(0, 255, 0), 5);

//----------------Get Block Rect and detection (Main)
//    Rect r_b[g_omr.n_180];
//    for(int i=0;i<g_omr.n_180;i++) {
//        r_b[i].x = g_omr.b_180[i].p1.x - g_omr.header.gap_x;
//        r_b[i].y = g_omr.b_180[i].p1.y - g_omr.header.gap_y;;
//        r_b[i].width = g_omr.b_180[i].p2.x - g_omr.b_180[i].p1.x;
//        r_b[i].height = g_omr.b_180[i].p2.y - g_omr.b_180[i].p1.y;
//
//        rectangle(matInput, r_b[i], Scalar(0, 255, 0), 5);
//        blob.clear();
//        blob = get_blob(matResult, r_b[i]);
//        for (int j=0; j<blob.size(); j++){
//            Point pt = blob[j];
//            circle(matInput, Point(pt.x, pt.y), 10, Scalar(255,0,0), 5);
//        }
//    }

    Rect r_b[g_omr.n_100];
    for(int i=0;i<g_omr.n_100;i++) {
        r_b[i].x = g_omr.b_100[i].p1.x - g_omr.header.gap_x;
        r_b[i].y = g_omr.b_100[i].p1.y - g_omr.header.gap_y;;
        r_b[i].width = g_omr.b_100[i].p2.x - g_omr.b_100[i].p1.x;
        r_b[i].height = g_omr.b_100[i].p2.y - g_omr.b_100[i].p1.y;

//        int r = g_omr.b_100[i].row;
//        int c = g_omr.b_100[i].col;
//        int st_x = int(r_b[i].height / (r+1));
//
//        for(int ri = 1; ri<=r; ri++) {
//            Point pt1 = Point(r_b[i].x, r_b[i].y + st_x * ri);
//            Point pt2 = Point(r_b[i].x + r_b[i].width, r_b[i].y + st_x * ri);
//            line(matInput, pt1, pt2, Scalar(255,0,0), 2);
//        }

        rectangle(matInput, r_b[i], Scalar(0, 255, 0), 5);

        blob.clear();
        blob = get_blob(matResult, r_b[i]);
        for (int j=0; j<blob.size(); j++){
            Point pt = blob[j];
            circle(matInput, Point(pt.x, pt.y), 10, Scalar(255,0,0), 5);
        }
    }
    matInput.copyTo(matResult);
}
*/

/**
 * INPUT: in_img: Gray, outImg: RGB
 **/
int* recog_row(Mat& inputImg, Rect rcOrg, int nCols, Mat& outImg) {
    int* res_array = (int*)malloc(nCols * sizeof(int));
    memset(res_array, 0, nCols * sizeof(int));

    Mat rowImg, thres, temp;
    inputImg(rcOrg).copyTo(rowImg);

    std::vector<std::vector<Point> > contours;
    std::vector<Vec4i> hierarchy;
    std::vector<Point> approxCurve, docCnt;

    threshold( rowImg, thres, 127, 255, THRESH_BINARY_INV|THRESH_OTSU);
    Mat element(7, 7, CV_8U, Scalar(1));

    morphologyEx(thres, rowImg, MORPH_OPEN, element);
    morphologyEx(rowImg, thres, MORPH_CLOSE, element);
//    bitwise_not(mat, mat);

    findContours(thres, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

    std::vector<std::vector<Point> > contours_poly(contours.size());
    std::vector<Rect> boundRect(contours.size());
    std::vector<std::vector<Point>> questionCnt;

    for( int i = 0; i< contours.size(); i++ )
    {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 0.1, true);
        int w =  boundingRect(Mat(contours[i])).width;
        int h =  boundingRect(Mat(contours[i])).height;
        double ar = (double)w/h;

        if(hierarchy[i][3] == -1) //No parent
            if(w >=10 && h >=10 /* && ar < 1.2 && ar > 0.8 */)
                questionCnt.push_back(contours_poly[i]);
    }


//    char str[200];
//    sprintf(str,"t - %ld", questionCnt.size());
//    __android_log_write(ANDROID_LOG_DEBUG, "test", str);

    cvtColor(thres, rowImg, COLOR_GRAY2RGB);
    vector<Point> ret;
    for(int i=0; i<questionCnt.size(); i++){
        Rect r = boundingRect(Mat(questionCnt[i]));
        int cx = r.x + r.width/2, cy = r.y + r.height/2;
        int cx1 = rcOrg.x + r.x + r.width/2, cy1 = rcOrg.y + r.y + r.height/2;

        float gap = rcOrg.width/(float)nCols;

        for(int k=0;k<nCols;k++) {
            if(cx>=k*gap && cx<(k+1)*gap)
                res_array[k] = 1;
        }

        circle(outImg, Point(cx1, cy1), 15, Scalar(255,0,0), 2);
    }

//    rowImg.copyTo(outImg(rcOrg));
    return res_array;
}
/*
int* recog_row(Mat& inputImg, Rect rcOrg, int nCols, Mat& outImg)
{
   int* res_array = (int*)malloc(nCols * sizeof(int));
   memset(res_array, 0, nCols * sizeof(int));

   Mat rowImg, thres, temp;
   inputImg(rcOrg).copyTo(rowImg);
//    rectangle(outImg, rcOrg, Scalar(255, 0, 0), 1);

//    threshold(rowImg, thres, 0, 255, THRESH_BINARY|THRESH_OTSU);

   Ptr<cv::CLAHE> clahe = cv::createCLAHE();
   clahe->setClipLimit(4);
   clahe->apply(rowImg, rowImg);

   threshold(rowImg, rowImg, 0, 255, THRESH_BINARY|THRESH_OTSU);
//    adaptiveThreshold(rowImg, temp, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 7, 2);
//
//    int erosion_type = MORPH_RECT;
//    int erosion_size = 2;
//    Mat ele = getStructuringElement(erosion_type,
//                                    Size(2 * erosion_size + 1, 2 * erosion_size + 1),
//                                    Point(erosion_size, erosion_size));
//
//    erode(temp, rowImg, ele);

   int wwid = rowImg.cols, hhei = rowImg.rows;
   int* row_hist = (int*)malloc(wwid * sizeof(int));
   int y, x;

   // histogram
   for (x = 0; x < wwid; x++) {
       row_hist[x] = 0;
       for (y = 0; y < hhei; y++) {
           if (getValue(rowImg.data, wwid, y, x) == 0)
               row_hist[x] += 1;
       }
   }

   // separate each cols
   for (x = 0; x < wwid; x++) {
       if (row_hist[x] > 0) row_hist[x] = 1;
       else row_hist[x] = 0;
   }

   // hole pad
   for (x = 1; x < wwid - 1; x++) {
       if (row_hist[x] > 0) continue;
       if (row_hist[x - 1] > 0 && row_hist[x + 1] > 0)
           row_hist[x] = 1;
   }

   row_hist[wwid - 1] = 0;
   vector<int> col_posit;
   for (x = 0; x < wwid - 1; x++) {
       row_hist[x] = (row_hist[x] == row_hist[x + 1]) ? 0 : 1;
       if (row_hist[x]>0)
           col_posit.push_back(x);
   }

   int nNums = (int)col_posit.size();

   char str[200];
   sprintf(str,"%ld", col_posit.size());
   __android_log_write(ANDROID_LOG_DEBUG, "test", str);

   int sub_num = 0;
   for (x = 0; x < nNums / 2; x++) {
       Rect rc = Rect(Point(rcOrg.x + col_posit[2 * x], rcOrg.y + 0), Point(rcOrg.x + col_posit[2 * x + 1], rcOrg.y + hhei));
       Rect rc1 = Rect(Point(rcOrg.x + col_posit[2 * x], rcOrg.y + 0), Point(rcOrg.x + col_posit[2 * x + 1], rcOrg.y + hhei));
       if (nNums > 2*nCols && rc.width < 20) continue;

       rectangle(rowImg, rc, Scalar(255, 0, 0), 1);
//        // marks
//        int sums = 0;
//        for (int xx = col_posit[2 * x]; xx <= col_posit[2 * x + 1]; xx++) {
//            for (int yy = 0; yy <hhei; yy++) {
//                if (getValue(rowImg.data, wwid, yy, xx) == 0)
//                    sums += 1;
//            }
//        }
//        float thv = (float)sums / (float)rc.area();
//        if (thv > 0.3) {
//            res_array[sub_num++] = 1;
//            circle(outImg, Point(rcOrg.x + rc.x + rc.width / 2, rcOrg.y + rc.y + rc.height / 2), 3, Scalar(255, 0, 255), 3);
//        }
   }

   cvtColor(rowImg, rowImg, COLOR_GRAY2RGB);
   rowImg.copyTo(outImg(rcOrg));
   return res_array;
}
*/
/**
 * INPUT: in_img: Gray, outImg: RGB
 **/
int* recog_oneblock(Mat& in_img, Rect rcOrg, int nRows, int nCols, Mat& outImg)
{
    int* res_matrix = (int*)malloc(nRows*nCols * sizeof(int));
    memset(res_matrix, 0, nRows*nCols * sizeof(int));

    Mat tmp, binary_img;

    Rect inerRc(rcOrg.x + 10, rcOrg.y + 10, rcOrg.width - 20, rcOrg.height - 20);
    in_img(inerRc).copyTo(tmp);

    Mat kernel(3, 3, CV_32F, cv::Scalar(255));
    kernel.at<float>(1, 1) = 5.0f;
    kernel.at<float>(0, 1) = -1.0f;
    kernel.at<float>(2, 1) = -1.0f;
    kernel.at<float>(1, 0) = -1.0f;
    kernel.at<float>(1, 2) = -1.0f;

    tmp.convertTo(binary_img, -1, 0.5, 0);

//    adaptiveThreshold(tmp, binary_img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 21, 10);
//    blur(binary_img,binary_img, Size(5,5));
//    medianBlur(binary_img, binary_img, 1);
//    GaussianBlur(tmp, binary_img, Size(5,5), 0);
//    Canny(tmp, binary_img, 50, 200);
//    threshold(tmp, binary_img, 100, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

//    int erosion_type = MORPH_ELLIPSE;
//    int erosion_size = 2;
//    Mat ele = getStructuringElement(erosion_type,
//                                        Size(2 * erosion_size + 1, 2 * erosion_size + 1),
//                                        Point(erosion_size, erosion_size));
//    morphologyEx(binary_img, binary_img, MORPH_OPEN, ele, Point(-1,-1), 2);
//
    cvtColor(binary_img, binary_img, COLOR_GRAY2RGB);
    binary_img.copyTo(outImg(inerRc));

    int nWid = binary_img.cols;
    int nHei = binary_img.rows;
    int i, j;
    size_t nMemSize = nHei * sizeof(int);
    int* hist = (int*)malloc(nMemSize);

    // calculate histogram according horizontal direct
    for (i = 0; i < nHei; i++) {
        hist[i] = 0;
        for (j = 0; j < nWid; j++) {
            if (getValue(binary_img.data, nWid, i, j) == 0)
                hist[i] += 1;
        }
    }

    // separate each rows
    int t = 15;
    for (i = 0; i < nHei; i++) {
        if (hist[i] > t) hist[i] = 1;
        else hist[i] = 0;
    }
    hist[0] = 0;
    hist[nHei - 1] = 0;
    vector<int> posit;
    for (i = 0; i < nHei - 1; i++) {
        hist[i] = (hist[i] == hist[i + 1]) ? 0 : 1;
        if (hist[i]>0)
            posit.push_back(i);
    }

    char str[200];
    sprintf(str,"%ld rows detect!", posit.size()/2);
    __android_log_write(ANDROID_LOG_DEBUG, "test", str);

    int gap = 2;
    for (i = 0; i < posit.size() / 2; i++) {
        Rect roi1 = Rect(Point(inerRc.x+gap, inerRc.y + posit[2 * i] - gap), Point(inerRc.x + nWid-gap, inerRc.y + posit[2 * i + 1] + gap)); //Global

        sprintf(str,"y:%d - height: %d !", (int)roi1.y, roi1.height);

        __android_log_write(ANDROID_LOG_DEBUG, "test", str);

        int* row_mark = recog_row(in_img, roi1, nCols, outImg);
        for (int jj = 0; jj<nCols; jj++)
            res_matrix[i*nCols+jj] = row_mark[jj];
        free(row_mark);
    }
    free(hist);

    return res_matrix;
}