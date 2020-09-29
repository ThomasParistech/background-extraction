// ConsoleApplication1.cpp : définit le point d'entrée pour l'application console.
//
#include "stdafx.h"
#include <opencv/cv.h>
#include "opencv2/core/core.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <string>
#include <iostream>
#include "Collector.h"

/// Global Variables
int width, height;
int originalWidth, originalHeight;

const int thresholdMax = 1300;
const int radiusErosionMax = 30;
const int radBlurMax = 30;
int threshold = 200;
int radiusErosion = 4;
int radBlur = 6;

Collector collect;

void MyCallbackForTrackbar(uint8_t* img)
{
	cv::imshow("Verification", cv::Mat(height, width, CV_8UC3, img));
}
void MyCallbackForThresh(int thresh, void *userData)
{
	threshold = thresh;
	MyCallbackForTrackbar(collect.updateImg(threshold, radiusErosion));
}
void MyCallbackForErosion(int rad, void *userData)
{
	radiusErosion = rad;
	MyCallbackForTrackbar(collect.updateImg(threshold, radiusErosion));
}
void MyCallbackForBlur(int rad, void *userData)
{
	radBlur = rad;
	MyCallbackForTrackbar(collect.updateImg(threshold, radiusErosion, radBlur));
}

cv::Mat blurMat(cv::Mat & img, int radBlur) {
	RollingBoxfilter box(img.cols, img.rows);
	return cv::Mat(img.rows, img.cols, CV_8UC3, Collector::convertToByte(box.meanBlurNorm(Collector::convertToPoint3(img.data), 5)));
};

#if 1

int main() {
	cv::Mat img = cv::imread("images/MairieAmberieu/DSC_1352.JPG");
	cv::namedWindow("Verification");
	cv::imshow("Verification", img);
	cv::waitKey(0);

	cv::Mat blurImg = blurMat(img, 5);
	cv::imshow("Verification", blurImg);
	cv::waitKey(0);
	return 0;
}

#else
std::string path = "images/Test1/";

int main() {
	// load the images
	std::cout << "---Loading images---\n";
	std::vector<cv::String> fn;
	cv::glob((path + "*.JPG").c_str(), fn, false);
	size_t n = fn.size();
	std::vector<cv::Mat> imgs, resizedImgs;
	imgs.reserve(n);
	resizedImgs.reserve(n);
	for (size_t i = 0; i < n; i++) {
		cv::Mat src = cv::imread(fn[i]);
		imgs.push_back(src);
	}
	originalWidth = imgs[0].cols;
	originalHeight = imgs[0].rows;

	height = 400;
	width = int(originalWidth * double(height)/ originalHeight);

	for (size_t i = 0; i < n; i++) {
		cv::Mat dst;
		cv::resize(imgs[i], dst, cv::Size(width, height), cv::INTER_CUBIC);
		resizedImgs.push_back(dst);
	}
	
	std::cout << "   Images loaded\n";

	////////////////////////////////
	
	collect = Collector(originalWidth, width, originalHeight, height);
	for (int i = 0; i < n; i++) {
		collect.addImg(imgs[i].data, resizedImgs[i].data);
	}

	cv::namedWindow("Verification");
	for (int i = 0; i < n; i++) {
		MyCallbackForTrackbar(collect.beginImg(i, threshold, radiusErosion, radBlur));

		cv::createTrackbar("Seuil", "Verification", &threshold, thresholdMax, MyCallbackForThresh);
		cv::createTrackbar("Erosion", "Verification", &radiusErosion, radiusErosionMax, MyCallbackForErosion);
		cv::createTrackbar("Lissage", "Verification", &radBlur, radBlurMax, MyCallbackForBlur);

		cv::waitKey(0);

		collect.saveMask();
	}


	std::cout << "---Merging images---\n";
	cv::Mat result(height, width, CV_8UC3, collect.merge());
	cv::imwrite("result.jpg", result);
	std::cout << "   Images merged\n";

	cv::namedWindow("Result", cv::WINDOW_AUTOSIZE);// Create a window for display.
	cv::imshow("Result", result);                   // Show our image inside it.
	cv::waitKey(0);
	
	return 0;
}
#endif