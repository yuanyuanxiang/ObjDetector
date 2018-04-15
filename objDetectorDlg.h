
// objDetectorDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "pyCaller.h"
#include "FileReader.h"

// 图像标注
struct Tips
{
	float score;
	cv::Rect rect;
	Tips() { memset(this, 0, sizeof(Tips)); }
	Tips(const cv::Rect &rt, float s) : score(s), rect(rt) { }
};

// 媒体状态
enum MediaState
{
	STATE_DONOTHING = 0,			// 就绪
	STATE_DETECTING,				// 检测
	STATE_PLAYING,					// 播放
	STATE_PAUSE, 
};

#define Thread_Start    1			// 线程启动
#define Thread_Stop		0			// 线程停在

// 线程名称枚举
enum Thread
{
	_InitPyCaller = 0, 
	_DetectVideo, 
	_PlayVideo,
	_Max
};

// CobjDetectorDlg 对话框
class CobjDetectorDlg : public CDialogEx
{
// 构造
public:
	CobjDetectorDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_OBJDETECTOR_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	MediaState m_nMediaState;		// 媒体状态

	char m_strFile[_MAX_PATH];		// 文件路径

	CFileReader m_reader;			// 文件读取器

	pyCaller *m_py;					// python调用者

	bool m_bExit;					// 是否退出程序

	bool m_bOK;						// pyCaller是否就位

	CRect m_rtPaint;				// 绘图矩形

	HDC m_hPaintDC;					// 绘图HDC

	int m_nThreadState[_Max];		// 线程状态

	// 是否正在检测
	bool IsDetecting() const { return STATE_DETECTING == m_nMediaState; }

	// 是否正忙
	bool IsBusy() const { return STATE_DETECTING == m_nMediaState || STATE_PLAYING == m_nMediaState; }

	// 初始化python调用环境
	static void InitPyCaller(LPVOID param);
	// 检测视频
	static void DetectVideo(LPVOID param);
	// 显示检测结果
	static void PlayVideo(LPVOID param);
	// 进行目标检测
	std::vector<Tips> DoDetect(cv::Mat &m);

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	CStatic m_picCtrl;
	afx_msg void OnFileOpen();
	afx_msg void OnDestroy();
	afx_msg void OnObjDetect();
	afx_msg void OnFileClose();
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnUpdateFileOpen(CCmdUI *pCmdUI);
	afx_msg void OnUpdateObjDetect(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFileClose(CCmdUI *pCmdUI);
	afx_msg void OnFileQuit();
	afx_msg void OnEditPlay();
	afx_msg void OnUpdateEditPlay(CCmdUI *pCmdUI);
	afx_msg void OnEditPause();
	afx_msg void OnUpdateEditPause(CCmdUI *pCmdUI);
	afx_msg void OnEditStop();
	afx_msg void OnUpdateEditStop(CCmdUI *pCmdUI);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
