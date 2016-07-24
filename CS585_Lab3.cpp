/*	CS585_Lab2.cpp
*	CS440 Spring 2016
*	Progamming Assignment 1
*	Modifier: Ziyan Huo
*
*	-------------
*	This program introduces the following concepts:
*		a) Reading a stream of images from a webcamera, and displaying the video
*		b) Skin color detection
*		c) Background differencing
*		d) Visualizing motion history
*	--------------
*
*	--------------
*	Words from modifier:
*	This program is modified on the code of the second lab.
*	I did not change the filename because it would lead to many problem and it is very complicated to create a new project.
*	--------------
*/

#include "stdafx.h"
//opencv libraries
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/photo/photo.hpp"
//C++ standard libraries
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

//function declarations

/**
Function that returns the maximum of 3 integers
@param a first integer
@param b second integer
@param c third integer
*/
int myMax(int a, int b, int c);

/**
Function that returns the minimum of 3 integers
@param a first integer
@param b second integer
@param c third integer
*/
int myMin(int a, int b, int c);

/**
Function that detects whether a pixel belongs to the skin based on RGB values
@param src The source color image
@param dst The destination grayscale image where skin pixels are colored white and the rest are colored black
*/
void mySkinDetect(Mat& src, Mat& dst);

/**
Function that does frame differencing between the current frame and the previous frame
@param src The current color image
@param prev The previous color image
@param dst The destination grayscale image where pixels are colored white if the corresponding pixel intensities in the current
and previous image are not the same
*/
void myFrameDifferencing(Mat& prev, Mat& curr, Mat& dst);

/**
Function that accumulates the frame differences for a certain number of pairs of frames
@param mh Vector of frame difference images
@param dst The destination grayscale image to store the accumulation of the frame difference images
*/
void myMotionEnergy(vector<Mat> mh, Mat& dst);

/**
This is the function that includes all the codes I added.
It includes many sub-function.
*/
void  all(Mat& src);

int main()
{

	//----------------
	//a) Reading a stream of images from a webcamera, and displaying the video
	//----------------
	// For more information on reading and writing video: http://docs.opencv.org/modules/highgui/doc/reading_and_writing_images_and_video.html
	// open the video camera no. 0
	VideoCapture cap(0);

	// if not successful, exit program
	if (!cap.isOpened())
	{
		cout << "Cannot open the video cam" << endl;
		return -1;
	}

	//create a window called "MyVideoFrame0"
	//namedWindow("MyVideo0", WINDOW_AUTOSIZE);
	Mat frame0;

	// read a new frame from video
	bool bSuccess0 = cap.read(frame0);

	//if not successful, break loop
	if (!bSuccess0)
	{
		cout << "Cannot read a frame from video stream" << endl;
	}

	//show the frame in "MyVideo" window
	//imshow("MyVideo0", frame0);

	//create a window called "MyVideo"
	namedWindow("MyVideo", WINDOW_AUTOSIZE);
	//namedWindow("MyVideoMH", WINDOW_AUTOSIZE);
	namedWindow("Skin", WINDOW_AUTOSIZE);

	vector<Mat> myMotionHistory;
	Mat fMH1, fMH2, fMH3;
	fMH1 = Mat::zeros(frame0.rows, frame0.cols, CV_8UC1);
	fMH2 = fMH1.clone();
	fMH3 = fMH1.clone();
	myMotionHistory.push_back(fMH1);
	myMotionHistory.push_back(fMH2);
	myMotionHistory.push_back(fMH3);

	while (1)
	{
		// read a new frame from video
		Mat frame;
		bool bSuccess = cap.read(frame);

		//if not successful, break loop
		if (!bSuccess)
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		// destination frame
		Mat frameDest;
		frameDest = Mat::zeros(frame.rows, frame.cols, CV_8UC1); //Returns a zero array of same size as src mat, and of type CV_8UC1
		//----------------
		//	b) Skin color detection
		//----------------
		mySkinDetect(frame, frameDest);
		imshow("Skin", frameDest);


		//
		//
		// call to the function all
		all(frameDest);
		//
		//
		//


		//----------------
		//	c) Background differencing
		//----------------


		//call myFrameDifferencing function
		myFrameDifferencing(frame0, frame, frameDest);
		imshow("MyVideo", frame);
		myMotionHistory.erase(myMotionHistory.begin());
		myMotionHistory.push_back(frameDest);
		Mat myMH = Mat::zeros(frame0.rows, frame0.cols, CV_8UC1);

		//----------------
		//  d) Visualizing motion history
		//----------------

		//call myMotionEnergy function
		myMotionEnergy(myMotionHistory, myMH);


		//imshow("MyVideoMH", myMH); //show the frame in "MyVideo" window
		frame0 = frame;
		//wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		if (waitKey(30) == 27)
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}

	}
	cap.release();
	return 0;
}

//Function that returns the maximum of 3 integers
int myMax(int a, int b, int c) {
	int m = a;
	(void)((m < b) && (m = b));
	(void)((m < c) && (m = c));
	return m;
}

//Function that returns the minimum of 3 integers
int myMin(int a, int b, int c) {
	int m = a;
	(void)((m > b) && (m = b));
	(void)((m > c) && (m = c));
	return m;
}

//Function that detects whether a pixel belongs to the skin based on RGB values
void mySkinDetect(Mat& src, Mat& dst) {
	//Surveys of skin color modeling and detection techniques:
	//Vezhnevets, Vladimir, Vassili Sazonov, and Alla Andreeva. "A survey on pixel-based skin color detection techniques." Proc. Graphicon. Vol. 3. 2003.
	//Kakumanu, Praveen, Sokratis Makrogiannis, and Nikolaos Bourbakis. "A survey of skin-color modeling and detection methods." Pattern recognition 40.3 (2007): 1106-1122.
	for (int i = 0; i < src.rows; i++){
		for (int j = 0; j < src.cols; j++){
			//For each pixel, compute the average intensity of the 3 color channels
			Vec3b intensity = src.at<Vec3b>(i, j); //Vec3b is a vector of 3 uchar (unsigned character)
			int B = intensity[0]; int G = intensity[1]; int R = intensity[2];
			if ((R > 95 && G > 40 && B > 20) && (myMax(R, G, B) - myMin(R, G, B) > 15) && (abs(R - G) > 15) && (R > G) && (R > B)){
				dst.at<uchar>(i, j) = 255;
			}
		}
	}
}

//Function that does frame differencing between the current frame and the previous frame
void myFrameDifferencing(Mat& prev, Mat& curr, Mat& dst) {
	//For more information on operation with arrays: http://docs.opencv.org/modules/core/doc/operations_on_arrays.html
	//For more information on how to use background subtraction methods: http://docs.opencv.org/trunk/doc/tutorials/video/background_subtraction/background_subtraction.html
	absdiff(prev, curr, dst);
	Mat gs = dst.clone();
	cvtColor(dst, gs, CV_BGR2GRAY);
	dst = gs > 50;
	Vec3b intensity = dst.at<Vec3b>(100, 100);
}

//Function that accumulates the frame differences for a certain number of pairs of frames
void myMotionEnergy(vector<Mat> mh, Mat& dst) {
	Mat mh0 = mh[0];
	Mat mh1 = mh[1];
	Mat mh2 = mh[2];

	for (int i = 0; i < dst.rows; i++){
		for (int j = 0; j < dst.cols; j++){
			if (mh0.at<uchar>(i, j) == 255 || mh1.at<uchar>(i, j) == 255 || mh2.at<uchar>(i, j) == 255){
				dst.at<uchar>(i, j) = 255;
			}
		}
	}
}



// This is where the function all begins.
RNG rng(12345);
VideoCapture cap;
int fcount = 0;
void all(Mat& src) {

	// Find contours and convex hulls, following code adapted from http://docs.opencv.org/2.4/doc/tutorials/imgproc/shapedescriptors/hull/hull.html
	Mat src_copy = src.clone();
	Mat threshold_output;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	threshold(src, threshold_output, 100, 255, THRESH_BINARY);

	findContours(threshold_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

	vector<vector<Point> >hull(contours.size());
	vector<vector<int> >inthull(contours.size());
	vector<vector<Vec4i> >defects(contours.size());

	for (size_t i = 0; i < contours.size(); i++)
	{
		convexHull(Mat(contours[i]), hull[i], false);
		convexHull(Mat(contours[i]), inthull[i], false);
	}
	Mat drawing = Mat::zeros(threshold_output.size(), CV_8UC3);

	// Find largest contour, adapted from lab3 code
	double maxsize = 0;
	int maxind = 0;
	Rect boundrec;
	for (int i = 0; i < contours.size(); i++)
	{
		// Documentation on contourArea: http://docs.opencv.org/modules/imgproc/doc/structural_analysis_and_shape_descriptors.html#
		int area = contourArea(contours[i]);
		if (area > maxsize) {
			maxsize = area;
			maxind = i;
		}
	}

	// Drawing contours and convexhulls
	Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
	drawContours(drawing, contours, maxind, color, 1, 8, vector<Vec4i>(), 0, Point());
	drawContours(drawing, hull, maxind, color, 1, 8, vector<Vec4i>(), 0, Point());

	// Find convexity defects using contours and convex hull
	convexityDefects(contours[maxind], inthull[maxind], defects[maxind]);

	/// Draw convexityDefects, adapted from http://stackoverflow.com/questions/31354150/opencv-convexity-defects-drawing
	int count = 0;
	for (int j = 0; j<defects[maxind].size(); ++j)
	{
		const Vec4i& v = defects[maxind][j];
		//float depth = v[3] / 256;
		if (v[3] > 12000) // filter defects by depth
		{
			int startidx = v[0]; Point ptStart(contours[maxind][startidx]);
			int endidx = v[1]; Point ptEnd(contours[maxind][endidx]);
			int faridx = v[2]; Point ptFar(contours[maxind][faridx]);

			//line(drawing, ptStart, ptEnd, Scalar(0, 255, 0), 1);
			//line(drawing, ptStart, ptFar, Scalar(0, 255, 0), 1);
			//line(drawing, ptEnd, ptFar, Scalar(0, 255, 0), 1);
			circle(drawing, ptFar, 3, Scalar(0, 0, 255), 2);

			count++;
		}
	}
	putText(drawing, "Count: " + to_string(count), Point(30, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255));

	namedWindow("Defect", WINDOW_AUTOSIZE);
	imshow("Defect", drawing);

	// Display a series of images based on # of defects detected
	Mat pic;
	namedWindow("cap", CV_WINDOW_AUTOSIZE);
	if (!cap.isOpened()) cap.open("vid/%02d.jpg");
	printf("%f", cap.get(CV_CAP_PROP_FRAME_COUNT));

	if (count >= 4 && fcount != cap.get(CV_CAP_PROP_FRAME_COUNT)) {
		waitKey(50);
		fcount++;
		cap.read(pic);
		imshow("cap", pic);
		waitKey(50);
	}
}