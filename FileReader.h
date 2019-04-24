#pragma once

#include <exception>
#include <queue>
#include "IPCamVisitor.h"

// 文件类型
enum FileType
{
	TYPE_UNKNOWN = 0,		// 未知
	TYPE_IMAGE,				// 图像
	TYPE_VIDEO,				// 视频
	TYPE_IPC,				// IPC数据流
	TYPE_CAMERA,			// 计算机摄像头
};

// 图像维度枚举
enum DIMS
{
	IMAGE_HEIGHT = 0,		// 高
	IMAGE_ROWS = 0,			// 行
	IMAGE_WIDTH = 1,		// 宽
	IMAGE_COLS = 1,			// 列
	IMAGE_CHANNEL = 2,		// 通道(每像素字节数)
};

#define BUF_MIN_LEN 8		// 最小缓存帧数

/************************************************************************
* @class CFileReader
* @brief 文件/流 读取器
************************************************************************/
class CFileReader
{
private:
	FileType m_nType;					// 文件类型
	int m_nDims[3];						// 图像维度
	int m_nImageBuf;					// 目标识别缓存帧数
	int m_nStreamBuf;					// 视频流缓存帧数
	std::queue<cv::Mat> m_Buffer;		// 缓存队列
	std::queue<cv::Mat> m_Stream;		// IPC Stream队列
	VideoCapture m_Cap;					// 视频读取器
	IPCamVisitor m_IPC;					// IPC 读取器

	CRITICAL_SECTION m_cs;
	void Lock() { EnterCriticalSection(&m_cs); }
	void Unlock() { LeaveCriticalSection(&m_cs); }

	// 定时抓取IPC图像的线程
	static void CaptureThread(LPVOID param);

	// 线程是否启用
	bool m_bThreadStart;

	// 向IPC Stream 添加一帧图像
	bool PushStream(const cv::Mat &m);

	// 从IPC Stream 取出一帧图像
	cv::Mat PopStream();

	// 没有取到视频画面
	bool NoStream();

	// 从摄像头读取一帧图像
	cv::Mat ReadCamera() { cv::Mat m; m_Cap.read(m); return m; }

public:
	CFileReader(void);
	~CFileReader(void);

	// 启动线程
	void StartThread();

	// 设置最大缓存帧数
	void SetBufferSize(int nSize); 

	// 获取图像维度
	int dims(DIMS n) const { return m_nDims[n]; }

	// 向播放队列添加一帧图像
	bool PushImage(const cv::Mat &m);

	// 从播放队列取出一帧图像
	cv::Mat PopImage();

	// 播放图像
	cv::Mat PlayVideo();

	// 是否为图像
	bool IsImage() const { return TYPE_IMAGE == m_nType; }

	// 是否为视频
	bool IsVideo() const { return m_nType > TYPE_IMAGE; }

	// 是否为本地文件
	bool IsFile() const { return m_nType < TYPE_IPC; }

	// 是否为视频流
	bool IsStream() const { return m_nType > TYPE_VIDEO; }

	// IPC
	bool IsIPC() const { return TYPE_IPC == m_nType; }

	// 是否为空
	bool IsEmpty() const { return TYPE_UNKNOWN == m_nType; }

	// 打开文件
	bool Open(const char *path);

	// 打开IP摄像机
	bool OpenIPCamera(const IPCamInfo &cam, HWND hWnd);

	// 清理
	void Clear();

	// 销毁
	void Destroy();

	// 是否正在缓存中
	bool IsBuffering();

	// 获取第一帧图像
	cv::Mat Front();
};
