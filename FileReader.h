#pragma once

#include <exception>
#include "CvvImage.h"
#include <queue>

// 文件类型
enum FileType
{
	TYPE_UNKNOWN = 0,
	TYPE_IMAGE, 
	TYPE_VIDEO, 
};

class CFileReader
{
private:
	FileType m_nType;
	int m_nDims[3];
	int m_nMaxBuf;
	CvvImage m_Image;
	std::queue<const cv::Mat> m_Buffer;
	VideoCapture m_Cap;

	CRITICAL_SECTION m_cs;
	void Lock() { EnterCriticalSection(&m_cs); }
	void Unlock() { LeaveCriticalSection(&m_cs); }

public:
	CFileReader(void);
	~CFileReader(void);

	int dims(int n) const { return m_nDims[n]; }

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
	bool IsVideo() const { return TYPE_VIDEO == m_nType; }

	// 播放队列是否为空
	bool IsEmpty();

	// 打开文件
	bool Open(const char *path);

	// 绘制图像
	void Draw(HDC hDc, CRect &rt);

	void Clear();

	void Destroy();

	// 是否正在缓存中
	bool IsBuffering();

	// 获取第一帧图像
	cv::Mat Front();
};
