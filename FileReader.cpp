#include "stdafx.h"
#include "FileReader.h"


CFileReader::CFileReader(void)
{
	m_nType = TYPE_IMAGE;
	m_nMaxBuf = 25;
	memset(m_nDims, 0, 3 * sizeof(int));
	InitializeCriticalSection(&m_cs);
}


CFileReader::~CFileReader(void)
{
	DeleteCriticalSection(&m_cs);
}


bool CFileReader::Open(const char *path)
{
	try
	{
		Clear();
		m_nType = TYPE_IMAGE;
		cv::Mat m = cv::imread(path);
		if (m.empty())
		{
			m_nType = TYPE_UNKNOWN;
			try {
				if (m_Cap.open(path) ? m_Cap.read(m) : false)
					m_nType = TYPE_VIDEO;
			}catch(...){
			}
		}
		if (TYPE_UNKNOWN != m_nType)
		{
			m_nDims[0] = m.rows;
			m_nDims[1] = m.cols;
			m_nDims[2] = m.step[1];
			PushImage(m);
			return true;
		}
	}catch(cv::Exception &e)
	{
	}
	return false;
}


bool CFileReader::OpenIPCamera(const IPCamInfo &cam, HWND hWnd)
{
	try
	{
		Clear();
		m_hWnd = hWnd;
		m_nType = TYPE_UNKNOWN;
		cv::Mat m;
		if (m_IPC.LoginCamera(cam, hWnd) >= 0)
		{
			m = m_IPC.GetCapture();
			m_nType = TYPE_IPC;
		}
		else
		{
			try {
				if (m_Cap.open(NULL) ? m_Cap.read(m) : false)
					m_nType = TYPE_CAMERA;
			}catch(...){
				return false;
			}
		}
		if (TYPE_UNKNOWN != m_nType)
		{
			m_nDims[0] = m.rows;
			m_nDims[1] = m.cols;
			m_nDims[2] = m.step[1];
			PushImage(m);
			return true;
		}
	}catch(cv::Exception &e)
	{
	}
	return false;
}


cv::Mat  CFileReader::PlayVideo()
{
	cv::Mat m;
	try
	{
		if (TYPE_VIDEO == m_nType || TYPE_CAMERA == m_nType)
		{
			if (m_Cap.read(m))
				return m;
		}
		else if (TYPE_IPC == m_nType)
			return m_IPC.GetCapture();

	}catch(...){
	}
	return m;
}


bool CFileReader::PushImage(const cv::Mat &m)
{
	Lock();
	bool result = m_Buffer.size() <= m_nMaxBuf;// 最多缓存帧
	if (result)
		m_Buffer.push(m);
	Unlock();
	return result;
}


int CFileReader::PopImage() // 返回当前帧数量
{
	if (TYPE_IMAGE == m_nType)
		return 1;
	Lock();
	bool bEmpty = m_Buffer.empty();
	if (!bEmpty)
		m_Buffer.pop();
	int nSize = m_Buffer.size();
	Unlock();
	return nSize;
}


// 绘制当前帧
void CFileReader::Draw(HDC hDc, CRect &rt)
{
	Lock();
	if (m_Buffer.empty() || m_Buffer.front().empty())
	{
		//FillRect(hDc, rt, CBrush(0xffffff));
		Unlock();
	}
	else 
	{
		IplImage t = IplImage(m_Buffer.front());
		m_Image.CopyOf(&t, 1);
		Unlock();
		m_Image.DrawToHDC(hDc, rt);
	}
}


void CFileReader::Clear()
{
	Lock();
	while(!m_Buffer.empty())
		m_Buffer.pop();
	Unlock();
	m_nType = TYPE_UNKNOWN;
	m_Cap.release();
	memset(m_nDims, 0, 3 * sizeof(int));
}


void CFileReader::Destroy()
{
	Clear();
	cv::destroyAllWindows();
}


bool CFileReader::IsEmpty()
{
	Lock();
	bool result = m_Buffer.empty() || m_Buffer.front().empty();
	Unlock();
	return result;
}


bool CFileReader::IsBuffering()
{
	Lock();
	bool result = m_Buffer.size() <= m_nMaxBuf;
	Unlock();
	return result;
}

cv::Mat CFileReader::Front()
{
	Lock();
	cv::Mat m;
	if (!m_Buffer.empty())
		m = m_Buffer.front();
	Unlock();
	return m;
}
