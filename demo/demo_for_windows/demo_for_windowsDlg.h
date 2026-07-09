
// demo_for_windowsDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "PTZButton.h"
#include "VideoDisplayWnd.h"
#include "VideoObjNetwork.h"

#define WM_UPDATE_CONNECTION_STATUS WM_USER + 150
#define WM_UPDATE_TELEMETRY WM_USER + 151
// Cdemo_for_windowsDlg 对话框
class Cdemo_for_windowsDlg : public CDialogEx
{
// 构造
public:
	Cdemo_for_windowsDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DEMO_FOR_WINDOWS_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnPTZBtnPressed(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPTZBtnReleased(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateConnStatus(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateTelemetry(WPARAM wParam, LPARAM lParam);
	afx_msg void OnClose();
public:
	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnBnClickedBtnOpenVideo();
	afx_msg void OnBnClickedBtnHome();
	afx_msg void OnBnClickedBtnQueryCfg();
private:
	static int VLK_ConnStatusCallback(int iConnStatus, const char* szMessage, int iMsgLen, void* pUserParam);
	static int VLK_DevStatusCallback(int iType, const char* szBuffer, int iBufLen, void* pUserParam);
private:
	CIPAddressCtrl m_GimbalIP;
	CEdit m_edTCPPort;
	int m_iPort;
	CComboBox m_cmbURL;

	CButton m_btnConnect;
	CButton m_btnOpenVideo;


	CPTZButton m_btnMoveUp;
	CPTZButton m_btnMoveLeft;
	CPTZButton m_btnMoveRight;
	CPTZButton m_btnMoveDown;
	CSliderCtrl m_sliderMoveSpeed;
	CString m_strDevSeries;


	CString m_strYaw;
	CString m_strPitch;
	CString m_strLat;
	CString m_strLng;
	CString m_strAlt;
	CString m_strZoom;
	CString m_strLaserDistance;
	CVideoDisplayWnd m_VideoDisplayWnd;

	CVideoObjNetwork* m_pVideoObj;
};
