#pragma once
#include "afxcmn.h"
#include "afxwin.h"


// CIPCConfigDlg 对话框

class CIPCConfigDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CIPCConfigDlg)

public:
	CIPCConfigDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CIPCConfigDlg();

// 对话框数据
	enum { IDD = IDD_IPCAMERA_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CNetAddressCtrl m_CtrlAddress;
	CEdit m_CtrlPort;
	CEdit m_CtrlUser;
	CEdit m_CtrlPassword;
	CString m_strAddress;
	int m_nPort;
	CString m_strUser;
	CString m_strPassword;
};
