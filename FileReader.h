#pragma once

#include <exception>
#include "CvvImage.h"
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

class CFileReader
{
private:
	FileType m_nType;					// 文件类型
	int m_nDims[3];						// 图像维度
	int m_nMaxBuf;						// 缓存帧数
	CvvImage m_Image;					// 绘图图像
	std::queue<const cv::Mat> m_Buffer;	// 缓存队列
	VideoCapture m_Cap;					// 视频读取器
	IPCamVisitor m_IPC;					// IPC
	HWND m_hWnd;

	CRITICAL_SECTION m_cs;
	void Lock() { EnterCriticalSection(&m_cs); }
	void Unlock() { LeaveCriticalSection(&m_cs); }

public:
	CFileReader(void);
	~CFileReader(void);

	// 获取图像维度
	int dims(int n) const { return m_nDims[n]; }

	// 获取最大缓存帧数
	int getMaxBuf() const { return m_nMaxBuf; }

	// 向播放队列添加一帧图像
	bool PushImage(const cv::Mat &m);

	// 从播放队列取出一帧图像
	int PopImage();

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

	// 播放队列是否为空
	bool IsEmpty();

	// 打开文件
	bool Open(const char *path);

	// 打开IP摄像机
	bool OpenIPCamera(const IPCamInfo &cam, HWND hWnd);

	// 绘制图像
	void Draw(HDC hDc, CRect &rt);

	// 清理
	void Clear();

	// 销毁
	void Destroy();

	// 是否正在缓存中
	bool IsBuffering();

	// 获取第一帧图像
	cv::Mat Front();
};
