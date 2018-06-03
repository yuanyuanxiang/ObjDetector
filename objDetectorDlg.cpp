
// objDetectorDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "objDetector.h"
#include "objDetectorDlg.h"
#include "afxdialogex.h"
#include "IPCConfigDlg.h"
#include <direct.h>
#include "SettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define EDGE 0

labelMap g_map;

// 根据当前时间生成一个文件标题[年-月-日 时分秒_毫秒]
std::string getTitle() 
{
	char title[64];
	time_t timep = time(NULL);
	strftime(title, sizeof(title), "%Y-%m-%d %H%M%S", localtime(&timep));
	sprintf_s(title, "%s_%d", title, clock());
	return title;
}

// 给图像添加矩形并标注文字
void AddRectanges(cv::Mat &m, const std::vector<Tips> &tips)
{
	for (std::vector<Tips>::const_iterator p = tips.begin(); 
		p != tips.end(); ++p)
		p->AddTips(m);
}


// 保存图像
void CobjDetectorDlg::Save(const cv::Mat &m, const vector<Tips> &tips)
{
	// 保存大图
	float p = rand() / (float)RAND_MAX;// 概率
	std::string s_Title = getTitle();
	const char *title = s_Title.c_str();
	const char *folder = p < 0.8f ? "train" : "test";
	char path[_MAX_PATH];
	sprintf_s(path, "%s\\%s\\%s.jpg", m_strImage, folder, title);
	cv::imwrite(path, m);
	// XML文件
	sprintf_s(path, "%s\\%s\\%s.xml", m_strImage, folder, title);
	FILE *file = fopen(path, "w+");
	char XML[4096] = { 0 };
	sprintf_s(XML, 
		"<annotation>\r\n"
		"  <folder>%s</folder>\r\n"			//1目录
		"  <filename>%s.jpg</filename>\r\n"	//2名称
		"  <path>%s.jpg</path>\r\n"			//3路径
		"  <source>\r\n"
		"    <database>Unknown</database>\r\n"
		"  </source>\r\n"
		"  <size>\r\n"
		"    <width>%d</width>\r\n"			//4宽
		"    <height>%d</height>\r\n"		//5高
		"    <depth>%d</depth>\r\n"			//6位深
		"  </size>\r\n"
		"  <segmented>0</segmented>\r\n"
		, folder, title, title, m.cols, m.rows, m.step[1]);
	int num = 0;// 小图编号
	for (vector<Tips>::const_iterator p = tips.begin(); p != tips.end(); ++p)
	{
		// 当前结果
		Tips tip = *p;
		// 保存小图
		folder = g_map.getItemName(tip.class_id);
		sprintf_s(path, "%s\\%s\\%s_%d.jpg", m_strImage, folder, title, ++num);
		cv::Rect rect = tip.rect;
		cv::Mat sub_m(m, rect);
		cv::imwrite(path, sub_m);
		// 保存xml
		char object[512];
		sprintf_s(object, 
			"  <object>\r\n"
			"    <name>%s</name>\r\n"			//7类别
			"    <pose>Unspecified</pose>\r\n"
			"    <truncated>0</truncated>\r\n"
			"    <difficult>0</difficult>\r\n"
			"    <bndbox>\r\n"
			"      <xmin>%d</xmin>\r\n"		//8
			"      <ymin>%d</ymin>\r\n"		//9
			"      <xmax>%d</xmax>\r\n"		//10
			"      <ymax>%d</ymax>\r\n"		//11
			"      <score>%.f</score>\r\n"	//12
			"    </bndbox>\r\n"
			"  </object>\r\n"
			, folder
			, rect.x, rect.y, rect.x + rect.width, rect.y + rect.height
			, tip.score);
		strcat(XML, object);
	}
	strcat(XML, "</annotation>\r\n");
	if (file)
	{
		fwrite(XML, strlen(XML), 1, file);
		fclose(file);
	}
}


// 初始化python调用环境
void CobjDetectorDlg::InitPyCaller(LPVOID param)
{
	CobjDetectorDlg *pThis = (CobjDetectorDlg*)param;
	pThis->m_nThreadState[_InitPyCaller] = Thread_Start;
	if (NULL == pThis->m_py)
		pThis->m_py = new pyCaller();
	pThis->m_bOK = pThis->m_py->Init("detect");
	pThis->m_nThreadState[_InitPyCaller] = Thread_Stop;
}


// 检测视频
void CobjDetectorDlg::DetectVideo(LPVOID param)
{
	OutputDebugStringA("======> DetectVideo Start.\n");
	CobjDetectorDlg *pThis = (CobjDetectorDlg*)param;
	pThis->m_nThreadState[_DetectVideo] = Thread_Start;
	timeBeginPeriod(1);
	// 延迟播放
	_beginthread(&PlayVideo, 0, param);
	std::vector<Tips> tips;
	int count = 0, flag = 0;
	const int& step = pThis->GetStep();
#ifdef _DEBUG
	clock_t last = clock(), cur;
#endif
	do 
	{
#ifdef _DEBUG
		cur = last;
#endif
		cv::Mat m = pThis->m_reader.PlayVideo();
		if (m.empty())
		{
			if (pThis->m_reader.IsFile())
				break;
			Sleep(10);
			continue;
		}
		switch (pThis->m_nMediaState)
		{
		case STATE_DETECTING:
			if ( 0 == (count % step) )
			{
				flag = 0;
				tips = pThis->DoDetect(m);
			}
			else if (++flag < min(step, 4))// 使矩形框停留 K * 40 ms
				AddRectanges(m, tips);
			break;
		case STATE_PAUSE:
			while(STATE_PAUSE == pThis->m_nMediaState)
				Sleep(10);
			break;
		case STATE_DONOTHING:
			break;
		default:
			break;
		}
		if (STATE_DONOTHING == pThis->m_nMediaState)
			break;
		++count;
		if (!pThis->m_reader.PushImage(m))// 播放不过来将丢帧处理
		{
			Sleep(10);
			continue;
		}
#ifdef _DEBUG
		last = clock();
		if (last - cur > 100)
			OUTPUT("======> DetectVideo time = %d, i = %d\n", last - cur, count);
#endif
	} while (!pThis->m_bExit);
	timeEndPeriod(1);
	pThis->m_nThreadState[_DetectVideo] = Thread_Stop;
	OutputDebugStringA("======> DetectVideo Stop.\n");
}


void CobjDetectorDlg::PlayVideo(LPVOID param)
{
	OutputDebugStringA("======> PlayVideo Start.\n");
	CobjDetectorDlg *pThis = (CobjDetectorDlg*)param;
	pThis->m_nThreadState[_PlayVideo] = Thread_Start;
	timeBeginPeriod(1);
	while(pThis->m_nThreadState[_DetectVideo] && pThis->m_reader.IsBuffering())
		Sleep(10);
#ifdef _DEBUG
	clock_t last = clock(), cur;
#endif
	do
	{
#ifdef _DEBUG
		cur = last;
#else 
		clock_t cur(clock());
#endif
		switch (pThis->m_nMediaState)
		{
		case STATE_DETECTING:
			break;
		case STATE_PAUSE:
			while(STATE_PAUSE == pThis->m_nMediaState)
				Sleep(10);
			break;
		case STATE_DONOTHING:
			break;
		default:
			break;
		}
		if (STATE_DONOTHING == pThis->m_nMediaState)
			break;
		cv::Mat m = pThis->m_reader.PopImage();
		if (m.empty())
		{
			Sleep(10);
			if (Thread_Stop == pThis->m_nThreadState[_DetectVideo])
				break;
			continue;
		}
		pThis->Paint(m);
		int nTime = 40 - (clock() - cur);
		Sleep(nTime > 0 ? nTime : 0);
#ifdef _DEBUG
		last = clock();
		if (last - cur > 45)
			OUTPUT("======> PlayVideo time = %d\n", last - cur);
#endif
	} while (!pThis->m_bExit);
	timeEndPeriod(1);
	pThis->m_reader.Open(pThis->m_strFile);
	pThis->Invalidate(TRUE);
	pThis->m_nMediaState = STATE_DONOTHING;
	pThis->m_nThreadState[_PlayVideo] = Thread_Stop;
	OutputDebugStringA("======> PlayVideo Stop.\n");
}


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CobjDetectorDlg 对话框



CobjDetectorDlg::CobjDetectorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CobjDetectorDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	memset(m_strFile, 0, _MAX_PATH);
	memset(m_strPath, 0, _MAX_PATH);
	memset(m_strImage, 0, _MAX_PATH);
	m_fThreshSave = 0.8;
	m_fThreshShow = 0.8;
	m_bOK = false;
	m_nMediaState = STATE_DONOTHING;
	m_bExit = false;
	m_nThreadState[_InitPyCaller] = m_nThreadState[_DetectVideo] = 
		m_nThreadState[_PlayVideo] = Thread_Stop;
}

void CobjDetectorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PICTURE, m_picCtrl);
}

BEGIN_MESSAGE_MAP(CobjDetectorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_FILE_OPEN, &CobjDetectorDlg::OpenFileProc)
	ON_WM_DESTROY()
	ON_COMMAND(ID_OBJ_DETECT, &CobjDetectorDlg::OnObjDetect)
	ON_COMMAND(ID_FILE_CLOSE, &CobjDetectorDlg::OnFileClose)
	ON_WM_INITMENUPOPUP()
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, &CobjDetectorDlg::OnUpdateFileOpen)
	ON_UPDATE_COMMAND_UI(ID_OBJ_DETECT, &CobjDetectorDlg::OnUpdateObjDetect)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE, &CobjDetectorDlg::OnUpdateFileClose)
	ON_COMMAND(ID_FILE_QUIT, &CobjDetectorDlg::OnFileQuit)
	ON_COMMAND(ID_EDIT_PLAY, &CobjDetectorDlg::OnEditPlay)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PLAY, &CobjDetectorDlg::OnUpdateEditPlay)
	ON_COMMAND(ID_EDIT_PAUSE, &CobjDetectorDlg::OnEditPause)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PAUSE, &CobjDetectorDlg::OnUpdateEditPause)
	ON_COMMAND(ID_EDIT_STOP, &CobjDetectorDlg::OnEditStop)
	ON_UPDATE_COMMAND_UI(ID_EDIT_STOP, &CobjDetectorDlg::OnUpdateEditStop)
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_IPC, &CobjDetectorDlg::OpenIPCProc)
	ON_UPDATE_COMMAND_UI(ID_FILE_IPC, &CobjDetectorDlg::OnUpdateFileIpc)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_SET_THRESHOLD, &CobjDetectorDlg::OnSetThreshold)
	ON_COMMAND(ID_SET_PYTHON, &CobjDetectorDlg::OnSetPython)
	ON_COMMAND(ID_SHOW_RESULT, &CobjDetectorDlg::ShowResultProc)
	ON_COMMAND(ID_FULL_SCREEN, &CobjDetectorDlg::FullScreenProc)
	ON_UPDATE_COMMAND_UI(ID_FULL_SCREEN, &CobjDetectorDlg::OnUpdateFullScreen)
END_MESSAGE_MAP()


// CobjDetectorDlg 消息处理程序

BOOL CobjDetectorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	CDC* pDC = m_picCtrl.GetDC();
	m_picCtrl.GetClientRect(&m_rtPaint);
	m_hPaintDC = pDC->GetSafeHdc();

	// 获取路径信息
	char path[_MAX_PATH], *p = path;
	GetModuleFileNameA(NULL, path, _MAX_PATH);
	while('\0' != *p) ++p;
	while('\\' != *p) --p;
	*p = '\0';
	strcpy_s(m_strPath, path);
	sprintf_s(m_strImage, "%s\\object_detection", m_strPath);
	if (-1 == _access(m_strImage, 0))
		_mkdir(m_strImage);
	// 创建训练及验证集目录
	char temp[_MAX_PATH];
	sprintf_s(temp, "%s\\%s", m_strImage, "train");
	if (-1 == _access(temp, 0))
		_mkdir(temp);
	sprintf_s(temp, "%s\\%s", m_strImage, "test");
	if (-1 == _access(temp, 0))
		_mkdir(temp);
	srand(time(NULL));
	// 读取类别信息
	strcpy(p, "\\label_map.ini");
	if (-1 == _access(path, 0))
		MessageBox(L"类别信息文件不存在!", L"提示", MB_ICONINFORMATION);
	int n = GetPrivateProfileIntA("item", "class_num", 1, path);
	g_map.Create(n);
	m_tf = tfOutput(max(n, 1));
	for (int i = 0; i < n; ++i)
	{
		char name[_MAX_PATH];
		sprintf_s(temp, "class%d", i+1);
		GetPrivateProfileStringA("item", temp, temp, name, _MAX_PATH, path);
		sprintf_s(temp, "class%d_id", i+1);
		int id = GetPrivateProfileIntA("item", temp, i+1, path);
		g_map.InsertItem(Item(name, id));
		sprintf_s(temp, "%s\\%s", m_strImage, name);
		if (-1 == _access(temp, 0))
			_mkdir(temp);
	}
	// 读取settings
	sprintf_s(path, "%s\\settings.ini", m_strPath);
	if (-1 == _access(path, 0))
		MessageBox(L"程序配置文件不存在!", L"提示", MB_ICONINFORMATION);
	GetPrivateProfileStringA("settings", "thresh_show", "0.8", temp, _MAX_PATH, path);
	m_fThreshShow = atof(temp);
	GetPrivateProfileStringA("settings", "thresh_save", "1.0", temp, _MAX_PATH, path);
	m_fThreshSave = atof(temp);
	m_fThreshSave = max(m_fThreshSave, m_fThreshShow);
	m_nBufSize = GetPrivateProfileIntA("settings", "buffer_size", 25, path);
	m_nDetectStep = GetPrivateProfileIntA("settings", "detect_step", 6, path);
	if (m_nDetectStep < 1)
		m_nDetectStep = 1;
	m_reader.SetBufferSize(m_nBufSize);

	GetPrivateProfileStringA("settings", "python_home", "", m_pyHome, _MAX_PATH, path);
	if(false == pyCaller::SetPythonHome(m_pyHome))
	{
		m_pyHome[0] = '\0';
		MessageBox(L"python_home配置错误!", L"提示", MB_ICONINFORMATION);
	}

	m_strSettings = CString(path);

#if USING_TENSORFLOW
	_beginthread(&InitPyCaller, 0, this);
#endif

	m_pResult = new CResultDlg(this);
	m_pResult->Create(IDD_RESULT_DLG, this);

	m_bFullScreen = false;
	GetDlgItem(IDC_IPC_CAPTURE)->ShowWindow(SW_HIDE);
	ShowWindow(SW_SHOWMAXIMIZED);
	ReSize();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CobjDetectorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CobjDetectorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		// 在Windows10 x64机器上即使不打开任何文件，激活对话框：
		// 此函数过于频繁使得程序空闲时CPU也很高，因此加上Sleep
		static clock_t tm = clock();
		if (!IsBusy())
			Paint(m_reader.Front());
		Sleep(clock() - tm < 20 ? 20 : 1);
		tm = clock();
		OUTPUT("======> OnPaint current time = %d\n", tm);
	}
}


// 绘制图像
void CobjDetectorDlg::Paint(const cv::Mat &m)
{
	if (!m.empty() && m_rtPaint.Width() && m_rtPaint.Height())
	{
#ifdef _DEBUG
		clock_t tm = clock();
#endif
		IplImage t = IplImage(m);
		m_Image.CopyOf(&t, 1);
		m_Image.DrawToHDC(m_hPaintDC, m_rtPaint);
#ifdef _DEBUG
		tm = clock() - tm;
		if (tm > 40)
			OUTPUT("======> DrawToHDC using time = %d\n", tm);
#endif
	}
}


//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CobjDetectorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CobjDetectorDlg::OpenFileProc()
{
	if (IsDetecting())
		return;
	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal())
	{
		m_nMediaState = STATE_DONOTHING;
		while(m_nThreadState[_PlayVideo] || m_nThreadState[_DetectVideo])
			Sleep(10);
		CString strFile = dlg.GetPathName();
		USES_CONVERSION;
		strcpy_s(m_strFile, W2A(strFile));
		if (!m_reader.Open(m_strFile))
		{
			TRACE("======> 打开文件失败.\n");
		}
		Invalidate(TRUE);
	}
}


void CobjDetectorDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	if (m_py) delete m_py;

	if (m_pResult) delete m_pResult;

	m_reader.Destroy();
}


void CobjDetectorDlg::OnObjDetect()
{
	ObjDetectProc();
}


void CobjDetectorDlg::ObjDetectProc()
{
	if(NULL == m_py || false == m_bOK)
		return;

	if (m_py)
		m_py->ActivateFunc("test_src");

	if (m_reader.IsImage())
	{
		DoDetect(m_reader.Front());
		Invalidate(TRUE);
	}else if (m_reader.IsVideo())
	{
		if (STATE_DONOTHING == m_nMediaState)
		{
			m_nMediaState = STATE_DETECTING;
			m_reader.StartThread();
			_beginthread(&DetectVideo, 0, this);
		}
		else if (STATE_PLAYING == m_nMediaState || STATE_PAUSE == m_nMediaState)
		{
			m_nMediaState = STATE_DETECTING;
		}
		else if(STATE_DETECTING == m_nMediaState)
			m_nMediaState = STATE_PLAYING;
	}
}


void CobjDetectorDlg::OnFileClose()
{
	memset(m_strFile, 0, _MAX_PATH);
	m_reader.Clear();
	m_pResult->ClearResult();
	Invalidate(TRUE);
	m_nMediaState = STATE_DONOTHING;
}


// 使菜单项的状态得以更新
void CobjDetectorDlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	CDialogEx::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	ASSERT(pPopupMenu != NULL);
	// Check the enabled state of various menu items.

	CCmdUI state;
	state.m_pMenu = pPopupMenu;
	ASSERT(state.m_pOther == NULL);
	ASSERT(state.m_pParentMenu == NULL);

	// Determine if menu is popup in top-level menu and set m_pOther to
	// it if so (m_pParentMenu == NULL indicates that it is secondary popup).
	HMENU hParentMenu;
	if (AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu)
		state.m_pParentMenu = pPopupMenu;    // Parent == child for tracking popup.
	else if ((hParentMenu = ::GetMenu(m_hWnd)) != NULL)
	{
		CWnd* pParent = this;
		// Child windows don't have menus--need to go to the top!
		if (pParent != NULL &&
			(hParentMenu = ::GetMenu(pParent->m_hWnd)) != NULL)
		{
			int nIndexMax = ::GetMenuItemCount(hParentMenu);
			for (int nIndex = 0; nIndex < nIndexMax; nIndex++)
			{
				if (::GetSubMenu(hParentMenu, nIndex) == pPopupMenu->m_hMenu)
				{
					// When popup is found, m_pParentMenu is containing menu.
					state.m_pParentMenu = CMenu::FromHandle(hParentMenu);
					break;
				}
			}
		}
	}

	state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax;
		state.m_nIndex++)
	{
		state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex);
		if (state.m_nID == 0)
			continue; // Menu separator or invalid cmd - ignore it.

		ASSERT(state.m_pOther == NULL);
		ASSERT(state.m_pMenu != NULL);
		if (state.m_nID == (UINT)-1)
		{
			// Possibly a popup menu, route to first item of that popup.
			state.m_pSubMenu = pPopupMenu->GetSubMenu(state.m_nIndex);
			if (state.m_pSubMenu == NULL ||
				(state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
				state.m_nID == (UINT)-1)
			{
				continue;       // First item of popup can't be routed to.
			}
			state.DoUpdate(this, TRUE);   // Popups are never auto disabled.
		}
		else
		{
			// Normal menu item.
			// Auto enable/disable if frame window has m_bAutoMenuEnable
			// set and command is _not_ a system command.
			state.m_pSubMenu = NULL;
			state.DoUpdate(this, FALSE);
		}

		// Adjust for menu deletions and additions.
		UINT nCount = pPopupMenu->GetMenuItemCount();
		if (nCount < state.m_nIndexMax)
		{
			state.m_nIndex -= (state.m_nIndexMax - nCount);
			while (state.m_nIndex < nCount &&
				pPopupMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
			{
				state.m_nIndex++;
			}
		}
		state.m_nIndexMax = nCount;
	}
}


void CobjDetectorDlg::OnUpdateFileOpen(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!IsDetecting());
}


void CobjDetectorDlg::OnUpdateObjDetect(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bOK && !m_reader.IsEmpty() && STATE_DETECTING != m_nMediaState);
}


void CobjDetectorDlg::OnUpdateFileClose(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_reader.IsEmpty());
}


void CobjDetectorDlg::OnFileQuit()
{
	m_pResult->StopShow();
	SendMessage(WM_CLOSE, 0, 0);
}


void CobjDetectorDlg::OnEditPlay()
{
	if (m_reader.IsVideo())
	{
		if (STATE_DONOTHING == m_nMediaState)
		{
			m_nMediaState = STATE_PLAYING;
			m_reader.StartThread();
			_beginthread(&DetectVideo, 0, this);
		}
		else if (STATE_PAUSE == m_nMediaState)
			m_nMediaState = STATE_PLAYING;
		else if (STATE_DETECTING == m_nMediaState)
			m_nMediaState = STATE_PLAYING;
	}
}


void CobjDetectorDlg::OnUpdateEditPlay(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_reader.IsVideo() && STATE_PLAYING != m_nMediaState);
}


void CobjDetectorDlg::OnEditPause()
{
	m_nMediaState = m_reader.IsStream() ? STATE_PLAYING : STATE_PAUSE;
}


void CobjDetectorDlg::OnUpdateEditPause(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_reader.IsVideo() && IsBusy());
}


void CobjDetectorDlg::OnEditStop()
{
	m_nMediaState = STATE_DONOTHING;
}


void CobjDetectorDlg::OnUpdateEditStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_reader.IsFile() && STATE_DONOTHING != m_nMediaState);
}


// 进行目标检测
std::vector<Tips> CobjDetectorDlg::DoDetect(cv::Mat &m)
{
	std::vector<Tips> tips;
	if (m_py && m_py->IsModuleLoaded())
	{
		// 需要转换为3通道图像再进行目标识别
		switch (m_reader.dims(IMAGE_CHANNEL))
		{
		case 3: break;
		case 1: cvtColor(m, m, CV_GRAY2RGB); break;
		case 4: cvtColor(m, m, CV_RGBA2RGB); break;
		case 2: default: return tips;
		}
		npy_intp dims[] = { m_reader.dims(IMAGE_ROWS), m_reader.dims(IMAGE_COLS), 3 }; // 给定维度信息
		// 生成包含这个多维数组的PyObject对象，使用PyArray_SimpleNewFromData函数
		// 第一个参数2表示维度，第二个为维度数组Dims,第三个参数指出数组的类型，第四个参数为数组
		PyObject *PyArray  = PyArray_SimpleNewFromData(3, dims, NPY_UBYTE, m.data);
		// 同样定义大小与Python函数参数个数一致的PyTuple对象
		PyObject *ArgArray = PyTuple_New(1);
		PyTuple_SetItem(ArgArray, 0, PyArray); 
		m_tf.zeros();
		m_tf = m_py->CallFunction("test_src", ArgArray, &m_tf);

		if (0 == m_tf.n)
			return tips;
		const int c = m.cols; // 列数
		const int r = m.rows; // 行数
		bool bNotFind = true; // 未检测到目标
		bool bSave = false; // 是否需要保存
		cv::Mat M = m.clone(); // 原始图像
		for (int k = 0; k < g_map.num; ++k)// 遍历每个类别
		{
			int next = k * MAX_BOXES_NUM;
			const float *p = m_tf.boxes + next * 4;
			for (int i = 0; i < m_tf.counts[k]; ++i)// 遍历每个结果
			{
				float y1 = *p++, x1 = *p++, y2 = *p++, x2 = *p++;

				cv::Rect rect(CvPoint(c * x1, r * y1), CvPoint(c * x2, r * y2));
				Tips tip(rect, m_tf.scores[next + i], m_tf.classes[next + i]);
				if (m_tf.scores[next + i] > m_fThreshShow)
				{
					if (0 == i)
					{
						if (m_tf.scores[next + i] > m_fThreshSave)
							bSave = true;
						if (m_pResult->IsWindowVisible())// 对相似度最大的结果进行展示
						{
							bNotFind = false;
							cv::Mat sub_m(m, rect);
							m_pResult->ShowResult(sub_m, m_tf.classes[next], m_tf.counts[k], m_tf.scores[next]);
						}
					}
					tip.AddTips(m);					// 显示
					tips.push_back(tip);
				}
			}
		}
		if (bSave)
			Save(M, tips);
		if (bNotFind)
			m_pResult->ClearResult();
	}
	return tips;
}


BOOL CobjDetectorDlg::PreTranslateMessage(MSG* pMsg)
{
	if( pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5: // 目标检测
			BeginWaitCursor();
			ObjDetectProc();
			EndWaitCursor();
			return TRUE;
		case VK_F11:// 全屏处理
			BeginWaitCursor();
			FullScreenProc();
			EndWaitCursor();
			return TRUE;
		case VK_ESCAPE: case VK_RETURN:// 屏蔽 ESC/Enter 关闭窗体
			return TRUE;
		default:
			break;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CobjDetectorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	return ReSize();
}


void CobjDetectorDlg::ReSize()
{
	CRect rt;
	GetClientRect(&rt);

	if (m_picCtrl.GetSafeHwnd())
	{
		m_picCtrl.MoveWindow(CRect(rt.left + EDGE, rt.top + EDGE, 
			rt.right - EDGE, rt.bottom - EDGE), TRUE);
		m_picCtrl.GetClientRect(&m_rtPaint);
	}
	if (m_pResult->GetSafeHwnd())
	{
		m_pResult->MoveWindow(CRect(rt.right - EDGE - 240*1.8f, 
			rt.bottom - EDGE - 128*2.4f, 
			rt.right - EDGE, rt.bottom - EDGE), TRUE);
	}
}


void CobjDetectorDlg::OpenIPCProc()
{
	if (IsDetecting())
		return;
	CIPCConfigDlg dlg;
	if (IDOK == dlg.DoModal())
	{
		m_nMediaState = STATE_DONOTHING;
		while(m_nThreadState[_PlayVideo] || m_nThreadState[_DetectVideo])
			Sleep(10);
		IPCamInfo info;
		USES_CONVERSION;
		strcpy_s(info.ip, W2A(dlg.m_strAddress));
		info.port = dlg.m_nPort;
		strcpy_s(info.user, W2A(dlg.m_strUser));
		strcpy_s(info.pwd, W2A(dlg.m_strPassword));
		HWND hWnd = GetDlgItem(IDC_IPC_CAPTURE)->GetSafeHwnd();
		if(!m_reader.OpenIPCamera(info, hWnd))
		{
			TRACE("======> 打开IPC失败.\n");
		}
		else{
			m_strFile[0] = '\0';
			OnEditPlay();
		}
		Invalidate(TRUE);
	}
}


void CobjDetectorDlg::OnUpdateFileIpc(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!IsDetecting());
}


BOOL CobjDetectorDlg::OnEraseBkgnd(CDC* pDC)
{
	return m_reader.IsImage() || STATE_DONOTHING == m_nMediaState ? 
		CDialogEx::OnEraseBkgnd(pDC) : TRUE;
}


void WriteConfigFileDouble(const CString &conf,  double f, const char *s)
{
	char buf[64];
	sprintf_s(buf, "%f", f);
	USES_CONVERSION;
	WritePrivateProfileString(L"settings", A2W(s), A2W(buf), conf);
}


void WriteConfigFileInt(const CString &conf,  int f, const char *s)
{
	char buf[64];
	sprintf_s(buf, "%d", f);
	USES_CONVERSION;
	WritePrivateProfileString(L"settings", A2W(s), A2W(buf), conf);
}


void CobjDetectorDlg::OnSetThreshold()
{
	CSettingsDlg dlg;
	dlg.m_fThreshSave = m_fThreshSave;
	dlg.m_fThreshShow = m_fThreshShow;
	dlg.m_nBufferSize = m_nBufSize;
	dlg.m_nDetectStep = m_nDetectStep;
	if (IDOK == dlg.DoModal())
	{
		if (m_fThreshShow != dlg.m_fThreshShow)
			WriteConfigFileDouble(m_strSettings, max(dlg.m_fThreshShow, 0.), "thresh_show");
		if (m_fThreshSave != dlg.m_fThreshSave)
			WriteConfigFileDouble(m_strSettings, max(dlg.m_fThreshSave, m_fThreshShow), "thresh_save");
		if (m_nBufSize != dlg.m_nBufferSize)
			WriteConfigFileInt(m_strSettings, dlg.m_nBufferSize, "buffer_size");
		if (m_nDetectStep != dlg.m_nDetectStep)
			WriteConfigFileInt(m_strSettings, dlg.m_nDetectStep, "detect_step");
		m_fThreshShow = max(dlg.m_fThreshShow, 0.);
		m_fThreshSave = max(dlg.m_fThreshSave, m_fThreshShow);
		m_nBufSize = dlg.m_nBufferSize;
		m_nDetectStep = dlg.m_nDetectStep;
		m_reader.SetBufferSize(m_nBufSize);
	}
}


void CobjDetectorDlg::OnSetPython()
{
	CString home(m_pyHome);
	CFolderPickerDialog dlg;
	if(IDOK == dlg.DoModal() && home != dlg.GetFolderPath())
	{
		USES_CONVERSION;
		const char *py = W2A(dlg.GetFolderPath());
		if(false == pyCaller::SetPythonHome(py))
		{
			MessageBox(L"python_home配置错误!", L"提示", MB_ICONINFORMATION);
		}
		else
		{
			sprintf_s(m_pyHome, py);
			WritePrivateProfileString(L"settings", L"python_home", dlg.GetFolderPath(), m_strSettings);
			if (m_py->IsModuleLoaded())
			{
				MessageBox(L"重启程序才能生效!", L"提示", MB_ICONINFORMATION);
			}
			else
			{
				static bool bInit = false;
				if (false == bInit)// 仅允许初始化一次
				{
					bInit = true;
#if USING_TENSORFLOW
					_beginthread(&InitPyCaller, 0, this);
#endif
				}
			}
		}
	}
}


void CobjDetectorDlg::ShowResultProc()
{
	m_pResult->ShowWindow(m_pResult->IsWindowVisible() ? SW_HIDE : SW_SHOW);
}


BOOL CobjDetectorDlg::DestroyWindow()
{
	m_bExit = true;
	m_nMediaState = STATE_DONOTHING;
	m_pResult->StopShow();
	while (m_nThreadState[_InitPyCaller] || m_nThreadState[_DetectVideo] || 
		m_nThreadState[_PlayVideo] || IsDetecting())
		Sleep(10);
	m_pResult->DestroyWindow();

	return CDialogEx::DestroyWindow();
}


// 全屏处理函数
void CobjDetectorDlg::FullScreenProc()
{
	if (false == m_bFullScreen)
	{
		//get current system resolution
		int g_iCurScreenWidth = GetSystemMetrics(SM_CXSCREEN);
		int g_iCurScreenHeight = GetSystemMetrics(SM_CYSCREEN);

		//for full screen while backplay
		GetWindowPlacement(&m_struOldWndpl);

		CRect rectWholeDlg;//entire client(including title bar)
		CRect rectClient;//client area(not including title bar)
		CRect rectFullScreen;
		GetWindowRect(&rectWholeDlg);
		RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &rectClient);
		ClientToScreen(&rectClient);

		rectFullScreen.left = rectWholeDlg.left-rectClient.left;
		rectFullScreen.top = rectWholeDlg.top-rectClient.top;
		rectFullScreen.right = rectWholeDlg.right+g_iCurScreenWidth - rectClient.right;
		rectFullScreen.bottom = rectWholeDlg.bottom+g_iCurScreenHeight - rectClient.bottom;
		//enter into full screen;
		WINDOWPLACEMENT struWndpl;
		struWndpl.length = sizeof(WINDOWPLACEMENT);
		struWndpl.flags = 0;
		struWndpl.showCmd = SW_SHOWNORMAL;
		struWndpl.rcNormalPosition = rectFullScreen;
		SetWindowPlacement(&struWndpl);

		m_bFullScreen = true;
	}else
	{
		SetWindowPlacement(&m_struOldWndpl);
		m_bFullScreen = false;
	}
}


void CobjDetectorDlg::OnUpdateFullScreen(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_reader.IsEmpty());
}
