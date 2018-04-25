#pragma once
#include "CvvImage.h"
#include "afxwin.h"

// 计时任务
#define TIME_SHOW 1

// CResultDlg 对话框
// 非模态对话框，用以具体结果的展现
class CResultDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CResultDlg)

public:
	CResultDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CResultDlg();

	// 对话框数据
	enum { IDD = IDD_RESULT_DLG };

	int m_nTimer;

	int m_nClassId;		// 当前类别ID

	HDC m_hPaintDC;
	CRect m_rtPaint;

	CvvImage m_Image;

	// 展示结果
	void ShowResult(const cv::Mat &m, int class_id, int counts, float scores);

	// 清除结果
	inline void ClearResult() { if(m_nCounts) { m_nCounts = 0; m_fScores = 0; FillRect(m_hPaintDC, m_rtPaint, CBrush(RGB(200, 200, 200))); } }

	// 停止展示
	inline void StopShow() { if(TIME_SHOW == m_nTimer) { KillTimer(m_nTimer); m_nTimer = 0; } ShowWindow(SW_HIDE); }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	CStatic m_Picture;
	CComboBox m_ComboClass;
	CEdit m_EditCounts;
	CEdit m_EditScores;
	int m_nCounts;
	float m_fScores;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
