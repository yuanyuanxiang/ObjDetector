#pragma once

#include "config.h"

#include "HC_SDK/HCNetSDK.h"
#include "HC_SDK/playm4.h"

#define WIDTH 2048

#define HEIGHT 1152

// IPC属性
typedef struct IPCamInfo
{
	int port;
	char ip[64];
	char user[128];
	char pwd[128];
	IPCamInfo() { memset(this, 0, sizeof(IPCamInfo)); }
	IPCamInfo(const char *_ip, int _port)
	{
		port = _port;
		strcpy_s(ip, _ip);
		strcpy_s(user, "admin");
		strcpy_s(pwd, "admin");
	}
}IPCamInfo;


/************************************************************************
* @class IPCamVisitor
* @brief IPC访问器
************************************************************************/
class IPCamVisitor
{
private:
	// 缓存大小
	int m_nLen;
	// 登陆ID
	LONG m_user;
	// 取流通道
	LONG m_port;

	// 图像缓存
	BYTE *m_buf;
	// RGB缓存
	BYTE *m_rgb;
	// 图像数据
	const BYTE *m_data;				// 指向图像缓存
	// 图像属性
	const BITMAPINFOHEADER *m_head;	// 指向图像缓存

	// IPC设备的信息
	IPCamInfo m_cam;

	// 缓存是否足够
	bool IsBufferEnough(int w, int h) const 
	{
		return sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + w * h * 4 <= m_nLen;
	}

public:
	IPCamVisitor();

	~IPCamVisitor(void);

	void Create(int nBufferLen = WIDTH * HEIGHT * 4);

	void Release();

	LONG LoginCamera(const IPCamInfo &info, HWND hWnd);

	void LogoutCamera();

	cv::Mat GetCapture();
};
