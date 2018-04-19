#include "stdafx.h"
#include "FileReader.h"


// 定时抓取IPC图像的线程
void CFileReader::CaptureThread(LPVOID param)
{
	CFileReader *pThis = (CFileReader*)param;
	OutputDebugStringA("======> CaptureThread Start.\n");
	pThis->m_bThreadStart = true;
	timeBeginPeriod(1);
	clock_t last = clock(), cur;
	while (pThis->IsStream())
	{
		cur = last;
		cv::Mat m = pThis->IsIPC() ? pThis->m_IPC.GetCapture() : 
			pThis->PlayVideo();
		if (m.empty())
		{
			Sleep(10);
			continue;
		}
		if(!pThis->PushStream(m))
		{
			Sleep(10);
			continue;
		}
		int nTime = 40 - (clock() - cur);
		Sleep(nTime > 0 ? nTime : 0);
		last = clock();
		if (last - cur > 45)
			OUTPUT("======> CaptureThread time = %d\n", last - cur);
	}
	timeEndPeriod(1);
	pThis->m_bThreadStart = false;
	OutputDebugStringA("======> CaptureThread Stop.\n");
}


// 启动线程
void CFileReader::StartThread() 
{
	if(IsStream() && !m_bThreadStart)
	{
		_beginthread(CaptureThread, 0, this);
		int k = 0;
		do {
			Sleep(40);
		}while (NoStream() && ++k < 25);
	}
}


CFileReader::CFileReader(void)
{
	m_nType = TYPE_IMAGE;
	m_nImageBuf = 50;
	m_nStreamBuf = 50;
	memset(m_nDims, 0, 3 * sizeof(int));
	m_bThreadStart = false;
	InitializeCriticalSection(&m_cs);
	m_IPC.Create();
}


CFileReader::~CFileReader(void)
{
	m_nType = TYPE_UNKNOWN;
	while (m_bThreadStart)
		Sleep(10);

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
		m_nType = TYPE_UNKNOWN;
		cv::Mat m;
		if (m_IPC.LoginCamera(cam, hWnd) >= 0)
		{
			int i = 0;
			do 
			{
				Sleep(40);
				m = m_IPC.GetCapture();
			} while (m.empty() && ++i < 25);
			
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
			return PopStream();

	}catch(...){
	}
	return m;
}


bool CFileReader::PushImage(const cv::Mat &m)
{
	Lock();
	bool result = m_Buffer.size() < m_nImageBuf;
	if (result)
		m_Buffer.push(m);
	Unlock();
	return result;
}


cv::Mat CFileReader::PopImage()
{
	cv::Mat m;
	Lock();
	if (!m_Buffer.empty())
	{
		m = m_Buffer.front();
		m_Buffer.pop();
	}
	Unlock();
	return m;
}


bool CFileReader::PushStream(const cv::Mat &m)
{
	Lock();
	bool result = m_Stream.size() < m_nStreamBuf;
	if (result)
		m_Stream.push(m);
	Unlock();
	return result;
}


cv::Mat CFileReader::PopStream()
{
	cv::Mat m;
	Lock();
	if (!m_Stream.empty())
	{
		m = m_Stream.front();
		m_Stream.pop();
	}
	Unlock();
	return m;
}


bool CFileReader::NoStream()
{
	Lock();
	bool result = m_Stream.empty();
	Unlock();
	return result;
}


void CFileReader::Clear()
{
	m_nType = TYPE_UNKNOWN;
	while (m_bThreadStart)
		Sleep(10);
	Lock();
	while(!m_Buffer.empty())
		m_Buffer.pop();
	while(!m_Stream.empty())
		m_Stream.pop();
	Unlock();
	m_Cap.release();
	memset(m_nDims, 0, 3 * sizeof(int));
}


void CFileReader::Destroy()
{
	Clear();
	cv::destroyAllWindows();
}


bool CFileReader::IsBuffering()
{
	Lock();
	bool result = m_Buffer.size() < m_nImageBuf;
	Unlock();
	return result;
}


cv::Mat CFileReader::Front()
{
	cv::Mat m;
	Lock();
	if (!m_Buffer.empty())
		m = m_Buffer.front();
	Unlock();
	return m;
}
