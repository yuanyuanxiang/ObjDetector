// 【注意】请将OpenCV的包含目录、python包含目录添加到项目附加包含目录
// 将python的库目录添加到项目的附加库目录
// 然后根据实际情况修改本文件。


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

#define USING_CV_LIB(LIB) CV_LIB_X(LIB, CV_VER) // 使用CV的库


//////////////////////////////////////////////////////////////////////////
// Python 配置
#define PYTHON_HOME L"D:/Anaconda3/envs/tfgpu"


//////////////////////////////////////////////////////////////////////////
#include "opencv2/opencv.hpp" 
#include "opencv/cv.h"
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


// 是否支持IPC
#define SUPPORT_IPC 1
