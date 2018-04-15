#include "pyCaller.h"

#ifdef _DEBUG
// 如果没有 Visual Leak Detector ，需注释此行
//#include "vld.h"
#endif

#define USING_OPENCV 1

#if USING_OPENCV

#include<opencv2/opencv.hpp>   
#include<opencv/cv.h> 
using namespace cv;

#define CV_VER "320"

#ifdef _DEBUG
#define CV_LIB_PATH "D:/opencv/opencv32/lib/Debug/"
#define CV_LIB_X(LIB, VER) CV_LIB_PATH##"opencv_"##LIB##VER##"d.lib"
#else
#define CV_LIB_PATH "D:/opencv/opencv32/lib/Release/"
#define CV_LIB_X(LIB, VER) CV_LIB_PATH##"opencv_"##LIB##VER##".lib"
#endif

#define USING_CV_LIB(LIB) CV_LIB_X(LIB, CV_VER)

#pragma comment(lib, USING_CV_LIB("core"))
#pragma comment(lib, USING_CV_LIB("highgui"))
#pragma comment(lib, USING_CV_LIB("imgproc"))
#pragma comment(lib, USING_CV_LIB("video"))
#pragma comment(lib, USING_CV_LIB("videoio"))
#pragma comment(lib, USING_CV_LIB("imgcodecs"))
#pragma comment(lib, USING_CV_LIB("photo"))

#endif


int main(int argc, const char *argv[])
{
	const char *path = argc == 1 ? "image.jpg" : argv[1];

	pyCaller py("detect");
	py.ActivateFunc("test_image");
	tfOutput output;
	for (int i = 0; i < 10; ++i)
	{
		output = py.CallFunction("test_image", path);
		output.PrintBoxes(0);
	}
	
	Mat im = imread(path);
	const float c = im.cols, r = im.rows;
	rectangle(im, cvPoint(c * output.p(0, 1), r * output.p(0, 0)), 
		cvPoint(c * output.p(0, 3), r * output.p(0, 2)), Scalar(255,0,0));
	imshow("object detection", im);
	waitKey(0);

	return 0;
}
