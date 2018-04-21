
// objDetectorDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "pyCaller.h"
#include "FileReader.h"
#include "CvvImage.h"

// 矩形框颜色
#define RECT_COLOR CV_RGB(0, 0, 255)
// 文字颜色
#define TEXT_COLOR CV_RGB(0, 255, 0)
// 线宽度
#define THICK_NESS 2

extern labelMap g_map; // 类别信息

// 图像标注
struct Tips
{
	int class_id;		// 类别ID
	float score;		// 得分
	cv::Rect rect;		// 矩形框
	Tips() { memset(this, 0, sizeof(Tips)); }
	Tips(const cv::Rect &rt, float s, int id = 1) : class_id(id), score(s), rect(rt) { }
	// 为图像添加注释
	void AddTips(cv::Mat &m) const
	{
		cv::rectangle(m, rect, RECT_COLOR, THICK_NESS);
		char text[256];
		sprintf_s(text, "%s:%.3f", g_map.getItemName(class_id), score);
		cv::putText(m, text, cvPoint(rect.x, rect.y), 
			CV_FONT_HERSHEY_SIMPLEX, 1.0, TEXT_COLOR, THICK_NESS);
	}
	void Output() const
	{
		OUTPUT("======> x=%d, y=%d, w=%d, h=%d), score = %f\n", rect.x, rect.y, rect.width, rect.height, score);
	}
};

// 媒体状态
enum MediaState
{
	STATE_DONOTHING = 0,			// 就绪
	STATE_DETECTING,				// 检测
	STATE_PLAYING,					// 播放
	STATE_PAUSE,					// 暂停
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

	char m_strPath[_MAX_PATH];		// 当前路径

	CFileReader m_reader;			// 文件读取器

	tfOutput m_tf;					// TensorFlow返回值

	pyCaller *m_py;					// python调用者

	bool m_bExit;					// 是否退出程序

	bool m_bOK;						// pyCaller是否就位

	CRect m_rtPaint;				// 绘图矩形

	HDC m_hPaintDC;					// 绘图HDC

	int m_nThreadState[_Max];		// 线程状态

	double m_fThreshSave;			// 图像保存阈值

	double m_fThreshShow;			// 图像显示阈值

	int m_nBufSize;					// 缓存帧数

	int m_nDetectStep;				// 检测帧数步长

	char m_pyHome[_MAX_PATH];		// Python Home

	CvvImage m_Image;				// 绘图图像

	CString m_strSettings;			// setting.ini

	// 是否正在检测
	bool IsDetecting() const { return STATE_DETECTING == m_nMediaState; }

	// 是否正忙
	bool IsBusy() const { return STATE_DETECTING == m_nMediaState || STATE_PLAYING == m_nMediaState; }

	const int& GetStep() const { return m_nDetectStep; }

	// 绘制图像
	void Paint(const cv::Mat &m);

	// 保存图像
	void Save(const cv::Mat &m, int class_id);

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
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnFileIpc();
	afx_msg void OnUpdateFileIpc(CCmdUI *pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSetThreshold();
	afx_msg void OnSetPython();
};
