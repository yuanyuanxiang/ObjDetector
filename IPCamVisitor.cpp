#include "stdafx.h"
#include "IPCamVisitor.h"

#if SUPPORT_IPC
#pragma comment(lib, "HC_SDK/HCCore.lib")
#pragma comment(lib, "HC_SDK/HCNetSDK.lib")
#pragma comment(lib, "HC_SDK/PlayCtrl.lib")
#pragma comment(lib, "ws2_32.lib")
#endif


IPCamVisitor::IPCamVisitor() 
{
	m_user = -1;
	m_port = -1;
	m_nLen = WIDTH * HEIGHT * 4;
	m_buf = new BYTE[m_nLen];
}


IPCamVisitor::~IPCamVisitor(void)
{
	LogoutCamera();
	delete [] m_buf;
}


// 登陆摄像机
LONG IPCamVisitor::LoginCamera(const IPCamInfo &info, HWND hWnd)
{
	LogoutCamera();
	memcpy(&m_cam, &info, sizeof(IPCamInfo));
#if SUPPORT_IPC
	// 初始化SDK
	if(!NET_DVR_Init())
	{
		OUTPUT("======> SDK Init failed.\n");
		return -1;
	}
	// 登录摄像机
	NET_DVR_DEVICEINFO_V30 devInfo = { 0 };
	m_user = NET_DVR_Login_V30(m_cam.ip, m_cam.port, m_cam.user, m_cam.pwd, &devInfo);
	m_user < 0 ? 
		OUTPUT("======> Login failed: %d. We will use computer camera.\n", NET_DVR_GetLastError()) : 
		OUTPUT("======> Login successfully. We will use IP camera %s: %d.\n", m_cam.ip, m_cam.port);

	NET_DVR_CLIENTINFO ClientInfo;
	ClientInfo.hPlayWnd = hWnd;		// 窗口为空，设备SDK不解码只取流
	ClientInfo.lChannel = 1;		// Channel number 设备通道号
	ClientInfo.lLinkMode = 0;		// Main Stream
	ClientInfo.sMultiCastIP = NULL;
	// 预览取流 
	int nHandle = NET_DVR_RealPlay_V30(m_user, &ClientInfo, NULL, NULL, TRUE);
	if (nHandle < 0)
	{
		OUTPUT("======> NET_DVR_RealPlay_V30 failed. Error number = %d.\n",NET_DVR_GetLastError());
	}else
	{
		m_port = NET_DVR_GetRealPlayerIndex(nHandle);
		if (m_port < 0)
			OUTPUT("======> NET_DVR_GetRealPlayerIndex failed. Error number = %d.\n",NET_DVR_GetLastError());
	}
#endif
	return m_user;
}


// 对网络摄像机进行捕获
cv::Mat IPCamVisitor::GetCapture()
{
	cv::Mat cap;
	int i = 0;
#if SUPPORT_IPC
	do 
	{
		if (m_port >= 0)
		{
			/** Parameters
			nPort[in] 播放通道号 
			pBitmap[in] 存放BMP图像数据地址，由用户分配，不得小于bmp图像大小，
			即sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + w * h * 4， 
			其中w和h分别为图像宽高
			nBufSize[in] 申请的缓冲区大小 
			pBmpSize [out] 获取到的实际bmp图像大小
			*/
			DWORD  pBmpSize = m_nLen;
			if(FALSE == PlayM4_GetBMP(m_port, m_buf, m_nLen, &pBmpSize))
			{
				OUTPUT("======> Capture failed. Error code = %d.\n", PlayM4_GetLastError(m_port));
			}
			// 两者长度相等，表示抓图为空
			if (m_nLen != pBmpSize)
			{
				BITMAPINFOHEADER i_header;
				memcpy(&i_header, m_buf + sizeof(BITMAPFILEHEADER), sizeof(BITMAPINFOHEADER));
				int width = i_header.biWidth, height = abs(i_header.biHeight); // 高度可能为负值
				cap = Mat(height, width, CV_8UC4, m_buf + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
			}
		}
		if (cap.empty())
		{
			if (100 == ++i)
				break;
			Sleep(40);
		}
	} while (cap.empty());
#endif
	return cap;
}


// 登出摄像机
void IPCamVisitor::LogoutCamera()
{
#if SUPPORT_IPC
	if (m_port >= 0)
	{
		NET_DVR_StopRealPlay(m_port);
		m_port = -1;
	}
	if (m_user >= 0)
	{
		NET_DVR_Logout_V30(m_user);
		m_user = -1;
	}
	NET_DVR_Cleanup();
#endif
}
