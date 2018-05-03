// ResultDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "objDetector.h"
#include "ResultDlg.h"
#include "afxdialogex.h"
#include "pyCaller.h"

extern labelMap g_map; // 类别信息

// CResultDlg 对话框

IMPLEMENT_DYNAMIC(CResultDlg, CDialogEx)

CResultDlg::CResultDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CResultDlg::IDD, pParent)
	, m_nTimer(TIME_SHOW)
	, m_nClassId(1)
	, m_nCounts(0)
	, m_fScores(0)
	, m_bShowContour(0)
{

}


CResultDlg::~CResultDlg()
{
}


void CResultDlg::ShowResult(const cv::Mat &m, int class_id, int counts, float scores)
{
	if (class_id == m_nClassId)
	{
		if (!m.empty())
		{
			cv::Mat gray;
			if (m_bShowContour)
			{
				cvtColor(m, gray, CV_BGR2GRAY);
				Canny(gray, gray, 125, 350);
				threshold(gray, gray, 128, 255, THRESH_BINARY);
			}else 
				gray = m;

			IplImage t = IplImage(gray);
			m_Image.CopyOf(&t, 1);
			m_Image.DrawToHDC(m_hPaintDC, m_rtPaint);
		}else
		{
			ClearResult();
		}
		m_nCounts = counts;
		m_fScores = scores;
	}
}


void CResultDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RESULT_PIC, m_Picture);
	DDX_Control(pDX, IDC_COMBO_CLASS, m_ComboClass);
	DDX_Control(pDX, IDC_EDIT_NUM, m_EditCounts);
	DDX_Control(pDX, IDC_EDIT_SCORE, m_EditScores);
	DDX_Text(pDX, IDC_EDIT_NUM, m_nCounts);
	DDX_Text(pDX, IDC_EDIT_SCORE, m_fScores);
}


BEGIN_MESSAGE_MAP(CResultDlg, CDialogEx)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_SHOW_CONTOUR, &CResultDlg::OnBnClickedShowContour)
END_MESSAGE_MAP()


// CResultDlg 消息处理程序

BOOL CResultDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 绘图相关
	CDC *pDC = m_Picture.GetDC();
	m_hPaintDC = pDC->GetSafeHdc();
	m_Picture.GetClientRect(&m_rtPaint);
	ASSERT(m_rtPaint.Width() && m_rtPaint.Height());

	// 控件相关
	USES_CONVERSION;
	for (int i = 0; i < g_map.num; ++i)
	{
		m_ComboClass.InsertString(i, A2W(g_map.getItemName(i+1)));
	}
	m_nClassId = 1;
	m_ComboClass.SetCurSel(m_nClassId - 1);
	timeBeginPeriod(1);
	// 40ms 刷新结果
	SetTimer(m_nTimer, 40, NULL);
	timeEndPeriod(1);

	// 使对话框具有透明效果
	SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE)^0x80000);
	HINSTANCE hInst = LoadLibrary(_T("User32.dll"));
	if (hInst)
	{
		typedef BOOL (WINAPI *MYFUN)(HWND, COLORREF, BYTE, DWORD);
		MYFUN fun = (MYFUN)GetProcAddress(hInst, "SetLayeredWindowAttributes");
		if (fun) fun(m_hWnd, RGB(0, 0, 0), 200, 0x2);
		FreeLibrary(hInst);
	}
	CButton *pBn = (CButton*)GetDlgItem(IDC_SHOW_CONTOUR);
	pBn->SetCheck(m_bShowContour);

	return TRUE;
}


void CResultDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (m_nTimer == nIDEvent)
	{
		CString buf;
		buf.Format(_T("%d"), m_nCounts);
		m_EditCounts.SetWindowText(buf);
		buf.Format(_T("%.6f"), m_fScores);
		m_EditScores.SetWindowText(buf);
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CResultDlg::OnBnClickedShowContour()
{
	m_bShowContour = !m_bShowContour;
	CButton *pBn = (CButton*)GetDlgItem(IDC_SHOW_CONTOUR);
	pBn->SetCheck(m_bShowContour);
}
