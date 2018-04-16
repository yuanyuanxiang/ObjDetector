
//////////////////////////////////////////////////////////////////////////
// OpenCV 配置
#define CV_VER "320"

#ifdef _DEBUG
#define CV_LIB_PATH "D:/opencv/opencv32/lib/Debug/"
#define CV_LIB_X(LIB, VER) CV_LIB_PATH##"opencv_"##LIB##VER##"d.lib"
#else
#define CV_LIB_PATH "D:/opencv/opencv32/lib/Release/"
#define CV_LIB_X(LIB, VER) CV_LIB_PATH##"opencv_"##LIB##VER##".lib"
#endif

#define USING_CV_LIB(LIB) CV_LIB_X(LIB, CV_VER)


//////////////////////////////////////////////////////////////////////////
// Python 配置
#define PYTHON_HOME L"D:/Anaconda3/envs/tfgpu"
// Tensorflow
#define USING_TENSORFLOW 1


//////////////////////////////////////////////////////////////////////////
// VDL 配置
#ifdef _DEBUG
#if !USING_TENSORFLOW
#include "vld.h" // 如果没有 Visual Leak Detector ，需注释此行
#endif
#endif


//////////////////////////////////////////////////////////////////////////
#include<opencv2/opencv.hpp>   
#include<opencv/cv.h> 
using namespace cv;

#pragma comment(lib, USING_CV_LIB("core"))
#pragma comment(lib, USING_CV_LIB("highgui"))
#pragma comment(lib, USING_CV_LIB("imgproc"))
#pragma comment(lib, USING_CV_LIB("video"))
#pragma comment(lib, USING_CV_LIB("videoio"))
#pragma comment(lib, USING_CV_LIB("imgcodecs"))
#pragma comment(lib, USING_CV_LIB("photo"))

#ifdef _AFX
#define OUTPUT TRACE
#else
#define OUTPUT printf
#endif
