// SettingsDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "objDetector.h"
#include "SettingsDlg.h"
#include "afxdialogex.h"


// CSettingsDlg 对话框

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialogEx)

CSettingsDlg::CSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSettingsDlg::IDD, pParent)
	, m_fThreshSave(0)
	, m_fThreshShow(0)
	, m_nBufferSize(0)
	, m_nDetectStep(0)
{

}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_THRESH_SAVE, m_ThreshSave);
	DDX_Control(pDX, IDC_EDIT_THRESH_SHOW, m_ThreshShow);
	DDX_Control(pDX, IDC_EDIT_BUFFER_SIZE, m_BufferSize);
	DDX_Control(pDX, IDC_EDIT_DETECT_STEP, m_DetectStep);
	DDX_Text(pDX, IDC_EDIT_THRESH_SAVE, m_fThreshSave);
	DDX_Text(pDX, IDC_EDIT_THRESH_SHOW, m_fThreshShow);
	DDX_Text(pDX, IDC_EDIT_BUFFER_SIZE, m_nBufferSize);
	DDV_MinMaxInt(pDX, m_nBufferSize, 0, 250);
	DDX_Text(pDX, IDC_EDIT_DETECT_STEP, m_nDetectStep);
	DDV_MinMaxInt(pDX, m_nDetectStep, 1, 100);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialogEx)
END_MESSAGE_MAP()


// CSettingsDlg 消息处理程序
