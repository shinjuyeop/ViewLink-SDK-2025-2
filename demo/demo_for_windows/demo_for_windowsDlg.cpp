
// demo_for_windowsDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "demo_for_windows.h"
#include "demo_for_windowsDlg.h"
#include "afxdialogex.h"
#include <vector>
#include <string>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

string Unicode2Ascii(LPCTSTR lpStr)
{
	int asciisize = ::WideCharToMultiByte(CP_OEMCP, 0, lpStr, -1, NULL, 0, NULL, NULL);
	if (asciisize == ERROR_NO_UNICODE_TRANSLATION)
	{
		throw std::exception("Invalid source string.");
	}
	if (asciisize == 0)
	{
		throw std::exception("Error in conversion.");
	}
	std::vector<char> resultstring(asciisize);
	int convresult = ::WideCharToMultiByte(CP_OEMCP, 0, lpStr, -1, &resultstring[0], asciisize, NULL, NULL);
	if (convresult != asciisize)
	{
		throw std::exception("convert fail.");
	}
	return string(&resultstring[0]);
}

wstring Ascii2Unicode(LPCSTR lpStr)
{
	int widesize = MultiByteToWideChar(CP_ACP, 0, lpStr, -1, NULL, 0);
	if (widesize == ERROR_NO_UNICODE_TRANSLATION)
	{
		throw std::exception("Invalid source string.");
	}
	if (widesize == 0)
	{
		throw std::exception("Error in conversion.");
	}
	std::vector<wchar_t> resultstring(widesize);
	int convresult = MultiByteToWideChar(CP_ACP, 0, lpStr, -1, &resultstring[0], widesize);
	if (convresult != widesize)
	{
		throw std::exception("convert fail.");
	}
	return std::wstring(&resultstring[0]);
}


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Cdemo_for_windowsDlg 对话框



Cdemo_for_windowsDlg::Cdemo_for_windowsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DEMO_FOR_WINDOWS_DIALOG, pParent)
	, m_iPort(0)
	, m_strDevSeries(_T(""))
	, m_strYaw(_T(""))
	, m_strPitch(_T(""))
	, m_strLat(_T(""))
	, m_strLng(_T(""))
	, m_strAlt(_T(""))
	, m_strZoom(_T(""))
	, m_strLaserDistance(_T(""))
	, m_pVideoObj(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cdemo_for_windowsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_CONNECT, m_btnConnect);
	DDX_Control(pDX, IDC_BTN_OPEN_VIDEO, m_btnOpenVideo);
	DDX_Control(pDX, IDC_GIMBAL_IP, m_GimbalIP);
	DDX_Control(pDX, IDC_EDIT_GIMBAL_TCP_PORT, m_edTCPPort);
	DDX_Control(pDX, IDC_CMB_URL, m_cmbURL);
	DDX_Text(pDX, IDC_EDIT_GIMBAL_TCP_PORT, m_iPort);
	DDX_Control(pDX, IDC_BTN_UP, m_btnMoveUp);
	DDX_Control(pDX, IDC_BTN_LEFT, m_btnMoveLeft);
	DDX_Control(pDX, IDC_BTN_RIGHT, m_btnMoveRight);
	DDX_Control(pDX, IDC_BTN_DOWN, m_btnMoveDown);
	DDX_Control(pDX, IDC_SLIDER_MOVE_SPEED, m_sliderMoveSpeed);
	DDX_Text(pDX, IDC_EDIT_DEV_SERIES, m_strDevSeries);
	DDX_Text(pDX, IDC_STATIC_YAW, m_strYaw);
	DDX_Text(pDX, IDC_STATIC_PITCH, m_strPitch);
	DDX_Text(pDX, IDC_STATIC_LAT, m_strLat);
	DDX_Text(pDX, IDC_STATIC_LNG, m_strLng);
	DDX_Text(pDX, IDC_STATIC_ALT, m_strAlt);
	DDX_Text(pDX, IDC_STATIC_ZOOM, m_strZoom);
	DDX_Text(pDX, IDC_STATIC_LASER_DISTANCE, m_strLaserDistance);
	DDX_Control(pDX, IDC_STATIC_VIDEO, m_VideoDisplayWnd);
}

BEGIN_MESSAGE_MAP(Cdemo_for_windowsDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONNECT, &Cdemo_for_windowsDlg::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_OPEN_VIDEO, &Cdemo_for_windowsDlg::OnBnClickedBtnOpenVideo)
	ON_BN_CLICKED(IDC_BTN_HOME, &Cdemo_for_windowsDlg::OnBnClickedBtnHome)
	ON_MESSAGE(WM_BUTTON_PRESSED, &Cdemo_for_windowsDlg::OnPTZBtnPressed)
	ON_MESSAGE(WM_BUTTON_RELEASED, &Cdemo_for_windowsDlg::OnPTZBtnReleased)
	ON_MESSAGE(WM_UPDATE_CONNECTION_STATUS, &Cdemo_for_windowsDlg::OnUpdateConnStatus)
	ON_MESSAGE(WM_UPDATE_TELEMETRY, &Cdemo_for_windowsDlg::OnUpdateTelemetry)
	ON_BN_CLICKED(IDC_BTN_QUERY_CFG, &Cdemo_for_windowsDlg::OnBnClickedBtnQueryCfg)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// Cdemo_for_windowsDlg 消息处理程序

BOOL Cdemo_for_windowsDlg::OnInitDialog()
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	av_register_all();
	avformat_network_init();

	m_GimbalIP.SetWindowText(_T("192.168.2.119"));
	m_iPort = 2000;

	m_cmbURL.AddString(_T("rtsp://192.168.2.119:554"));
	m_cmbURL.SetCurSel(0);


	m_sliderMoveSpeed.SetRange(1, 20);
	m_sliderMoveSpeed.SetPos(10);

	UpdateData(FALSE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void Cdemo_for_windowsDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Cdemo_for_windowsDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR Cdemo_for_windowsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

int Cdemo_for_windowsDlg::VLK_ConnStatusCallback(int iConnStatus, const char* szMessage, int iMsgLen, void* pUserParam)
{
	Cdemo_for_windowsDlg* pThis = (Cdemo_for_windowsDlg*)pUserParam;
	if (NULL == pThis)
	{
		return 0;
	}

	if (iConnStatus == VLK_CONN_STATUS_TCP_CONNECTED)
	{
		TRACE("################# conncet TCP Gimbal success !\n");
		::PostMessage(pThis->m_hWnd, WM_UPDATE_CONNECTION_STATUS, 1, 0);
	}
	else if (iConnStatus == VLK_CONN_STATUS_TCP_DISCONNECTED)
	{
		TRACE("################# conncet TCP Gimbal failed !\n");
		::PostMessage(pThis->m_hWnd, WM_UPDATE_CONNECTION_STATUS, 0, 0);
	}
	else if (iConnStatus == VLK_CONN_STATUS_SERIAL_PORT_CONNECTED)
	{
		TRACE("################# conncet serial port Gimbal success !\n");
		::PostMessage(pThis->m_hWnd, WM_UPDATE_CONNECTION_STATUS, 1, 0);
	}
	else if (iConnStatus == VLK_CONN_STATUS_SERIAL_PORT_DISCONNECTED)
	{
		TRACE("################# conncet serial port Gimbal failed !\n");
		::PostMessage(pThis->m_hWnd, WM_UPDATE_CONNECTION_STATUS, 0, 0);
	}

	return 0;
}

LRESULT Cdemo_for_windowsDlg::OnUpdateConnStatus(WPARAM wParam, LPARAM lParam)
{
	if (wParam != 0)
	{
		m_btnConnect.SetWindowText(_T("Disconnect"));
	}
	else
	{
		m_btnConnect.SetWindowText(_T("Connect"));
	}

	return 0;
}

int Cdemo_for_windowsDlg::VLK_DevStatusCallback(int iType, const char* szBuffer, int iBufLen, void* pUserParam)
{
	Cdemo_for_windowsDlg* pThis = (Cdemo_for_windowsDlg*)pUserParam;
	if (NULL == pThis)
	{
		return 0;
	}

	if (iType == VLK_DEV_STATUS_TYPE_MODEL)
	{
		if (iBufLen != sizeof(VLK_DEV_MODEL))
		{
			TRACE("################## bad device model info\n");
			return 0;
		}

		VLK_DEV_MODEL* pModel = (VLK_DEV_MODEL*)szBuffer;
		TRACE("model code: %02x, model name: %s\n", pModel->cModelCode, pModel->szModelName);
		pThis->m_strDevSeries = Ascii2Unicode(pModel->szModelName).c_str();
		::PostMessage(pThis->m_hWnd, WM_UPDATE_TELEMETRY, 0, 0);
	}
	else if (iType == VLK_DEV_STATUS_TYPE_CONFIG)
	{
		if (iBufLen != sizeof(VLK_DEV_CONFIG))
		{
			TRACE("################## bad device configure\n");
			return 0;
		}

		VLK_DEV_CONFIG* pDevConfig = (VLK_DEV_CONFIG*)szBuffer;
		TRACE("VersionNO: %s, DeviceID: %s, SerialNO: %s\n",
			pDevConfig->cVersionNO, pDevConfig->cDeviceID, pDevConfig->cSerialNO);

	}
	else if (iType == VLK_DEV_STATUS_TYPE_TELEMETRY)
	{
		if (iBufLen != sizeof(VLK_DEV_TELEMETRY))
		{
			TRACE("################## bad telemetry data\n");
			return 0;
		}

		VLK_DEV_TELEMETRY* pTelemetry = (VLK_DEV_TELEMETRY*)szBuffer;
		TRACE("Yaw: %lf, Pitch: %lf, Sensor type: %02x, Zoom mag times: %d\n",
			pTelemetry->dYaw, pTelemetry->dPitch, pTelemetry->emSensorType);

		pThis->m_strYaw.Format(_T("%lf"), pTelemetry->dYaw);
		pThis->m_strPitch.Format(_T("%lf"), pTelemetry->dPitch);
		pThis->m_strLat.Format(_T("%lf"), pTelemetry->dTargetLat);
		pThis->m_strLng.Format(_T("%lf"), pTelemetry->dTargetLng);
		pThis->m_strAlt.Format(_T("%lf"), pTelemetry->dTargetAlt);
		//pThis->m_strZoom.Format(_T("%d"), pTelemetry->sZoomMagTimes);
		pThis->m_strLaserDistance.Format(_T("%d"), pTelemetry->sLaserDistance);
		::PostMessage(pThis->m_hWnd, WM_UPDATE_TELEMETRY, 0, 0);
	}

	return 0;
}

LRESULT Cdemo_for_windowsDlg::OnUpdateTelemetry(WPARAM wParam, LPARAM lParam)
{
	UpdateData(FALSE);
	return 0;
}

void Cdemo_for_windowsDlg::OnBnClickedBtnConnect()
{
	// TODO: 在此添加控件通知处理程序代码
	if (VLK_IsConnected())
	{
		VLK_Disconnect();
		m_btnConnect.SetWindowText(_T("Connecting"));
	}
	else
	{
		UpdateData(TRUE);

		VLK_CONN_PARAM Param;
		memset(&Param, 0, sizeof(VLK_CONN_PARAM));
		Param.emType = VLK_CONN_TYPE_TCP;

		CString csIPAddress;
		m_GimbalIP.GetWindowText(csIPAddress);
		string strIPAddress = Unicode2Ascii(csIPAddress);
		strcpy_s(Param.ConnParam.IPAddr.szIPV4, 16, strIPAddress.c_str());
		Param.ConnParam.IPAddr.iPort = m_iPort;

		if (0 != VLK_Connect(&Param, VLK_ConnStatusCallback, this))
		{
			TRACE("VLK_Connect failed!\n");
		}

		VLK_RegisterDevStatusCB(VLK_DevStatusCallback, this);
		m_btnConnect.SetWindowText(_T("Connecting ..."));
	}
}

void Cdemo_for_windowsDlg::OnBnClickedBtnOpenVideo()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_pVideoObj)
	{
		m_pVideoObj->Close();
		delete m_pVideoObj;
		m_pVideoObj = NULL;

		m_btnOpenVideo.SetWindowText(_T("Open video"));
	}
	else
	{
		CString strURL;
		m_cmbURL.GetWindowText(strURL);
		if (!strURL.IsEmpty())
		{
			m_pVideoObj = new CVideoObjNetwork();
			string stlstrURL = Unicode2Ascii(strURL);
			m_pVideoObj->Open(stlstrURL.c_str(), m_VideoDisplayWnd.GetSafeHwnd());
			m_btnOpenVideo.SetWindowText(_T("Close video"));
		}
	}
}

void Cdemo_for_windowsDlg::OnBnClickedBtnHome()
{
	// TODO: 在此添加控件通知处理程序代码
	VLK_Home();
}

LRESULT Cdemo_for_windowsDlg::OnPTZBtnPressed(WPARAM wParam, LPARAM lParam)
{
	CPTZButton* pBtn = (CPTZButton*)wParam;
	double dSpeedRate = (double)m_sliderMoveSpeed.GetPos() / (double)m_sliderMoveSpeed.GetRangeMax();
	if (pBtn == &m_btnMoveUp)
	{
		double dScaleSpeed = (double)VLK_MAX_PITCH_SPEED * dSpeedRate;
		VLK_Move(0, dScaleSpeed);
	}
	else if (pBtn == &m_btnMoveLeft)
	{
		double dScaleSpeed = (double)VLK_MAX_YAW_SPEED * dSpeedRate * -1;
		VLK_Move(dScaleSpeed, 0);
	}
	else if (pBtn == &m_btnMoveRight)
	{
		double dScaleSpeed = (double)VLK_MAX_YAW_SPEED * dSpeedRate;
		VLK_Move(dScaleSpeed, 0);
	}
	else if (pBtn == &m_btnMoveDown)
	{
		double dScaleSpeed = (double)VLK_MAX_PITCH_SPEED * dSpeedRate * -1;
		VLK_Move(0, dScaleSpeed);
	}

	return 0L;
}

LRESULT Cdemo_for_windowsDlg::OnPTZBtnReleased(WPARAM wParam, LPARAM lParam)
{
	VLK_Stop();
	return 0L;
}


void Cdemo_for_windowsDlg::OnBnClickedBtnQueryCfg()
{
	// TODO: 在此添加控件通知处理程序代码
	VLK_QueryDevConfiguration();
}


void Cdemo_for_windowsDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_pVideoObj != NULL)
	{
		m_pVideoObj->Close();
		delete m_pVideoObj;
		m_pVideoObj = NULL;
	}

	VLK_Disconnect();

	CDialogEx::OnClose();
}
