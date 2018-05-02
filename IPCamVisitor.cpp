#include "stdafx.h"
#include "IPCamVisitor.h"

#if SUPPORT_IPC
#pragma comment(lib, "HC_SDK/HCCore.lib")
#pragma comment(lib, "HC_SDK/HCNetSDK.lib")
#pragma comment(lib, "HC_SDK/PlayCtrl.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

// 4字节对齐
#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)

IPCamVisitor::IPCamVisitor() 
{
	m_nLen = 0;
	m_user = -1;
	m_port = -1;
	m_buf = NULL;
	m_rgb = NULL;
	m_data = NULL;
	m_head = NULL;
#if SUPPORT_IPC
	// 初始化SDK
	if(!NET_DVR_Init())
	{
		OUTPUT("======> SDK Init failed.\n");
	}
#endif
}


IPCamVisitor::~IPCamVisitor(void)
{
	LogoutCamera();
#if SUPPORT_IPC
	if(!NET_DVR_Cleanup())
	{
		OUTPUT("======> SDK unInit failed.\n");
	}
#endif
	Release();
}


void IPCamVisitor::Create(int nBufferLen)
{
	m_nLen = max(nBufferLen, 4 * 1024 * 1024);
	m_buf = new BYTE[m_nLen];
	m_rgb = new BYTE[3 * m_nLen / 4];
	const BYTE* head = m_buf + sizeof(BITMAPFILEHEADER); // 图像信息位置
	m_head = (BITMAPINFOHEADER *) head;
	m_data = head + sizeof(BITMAPINFOHEADER); // 图像数据位置
}


void IPCamVisitor::Release()
{
	if (m_buf)
	{
		delete [] m_buf;
		m_nLen = 0;
		m_buf = NULL;
		m_head = NULL;
		m_data = NULL;
	}
	if (m_rgb)
	{
		delete [] m_rgb;
		m_rgb = NULL;
	}
}

// 登陆摄像机
LONG IPCamVisitor::LoginCamera(const IPCamInfo &info, HWND hWnd)
{
	LogoutCamera();
	memcpy(&m_cam, &info, sizeof(IPCamInfo));
#if SUPPORT_IPC
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
#if SUPPORT_IPC
	do 
	{
		if (m_port >= 0 && m_buf)
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
			else if (m_nLen != pBmpSize)
			{
				int W = m_head->biWidth, H = abs(m_head->biHeight);
				if (false == IsBufferEnough(W, H))
				{
					OUTPUT("======> Buffer of IPCamVisitor is not enough.\n");
					Release();
					break;
				}
				int srcLen = 4 * W, dstLen = WIDTHBYTES(24 * W);
				BYTE *pDst = m_rgb;
				const BYTE *pSrc = m_data;
				for (int i = 0; i < H; ++i, pSrc += srcLen, pDst += dstLen)
				{
					BYTE *pDstRow = pDst;
					const BYTE *pSrcRow = pSrc;
					for (int j = 0; j < W; ++j, pSrcRow += 4, pDstRow += 3)
					{
						memcpy(pDstRow, pSrcRow, 3);
					}
				}
				cap = Mat(H, W, CV_8UC3);
				memcpy(cap.data, m_rgb, H * dstLen);
				// 下述构造函数是浅拷贝, 参看mat.hpp 849行
				// cap = Mat(H, W, CV_8UC3, m_rgb);
			}
		}
	} while (false);
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
#endif
}
