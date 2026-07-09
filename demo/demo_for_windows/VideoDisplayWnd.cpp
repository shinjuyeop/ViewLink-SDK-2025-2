#include "stdafx.h"
#include "VideoDisplayWnd.h"
#include "VideoObjNetwork.h"

#define STOP_ZOOMING_TIMER_ID 1
CVideoDisplayWnd::CVideoDisplayWnd()
	: m_iVideoWidth(0), m_iVideoHeight(0)
{
}


CVideoDisplayWnd::~CVideoDisplayWnd()
{
}
BEGIN_MESSAGE_MAP(CVideoDisplayWnd, CStatic)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEHWHEEL()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDOWN()
	ON_WM_TIMER()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_MESSAGE(WM_VIDEO_RESOLUTION, &CVideoDisplayWnd::OnVideoResolution)
END_MESSAGE_MAP()


void CVideoDisplayWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	SetFocus();
	VLK_TRACK_MODE_PARAM param;
	param.emTrackTempSize = VLK_TRACK_TEMPLATE_SIZE_AUTO;
	param.emTrackSensor = VLK_SENSOR_VISIBLE1;

	if (m_iVideoWidth == 0 || m_iVideoHeight == 0)
	{
		return;
	}

	CRect rc;
	GetClientRect(rc);
	int iImgPosX = ((float)point.x / (float)rc.Width()) * m_iVideoWidth;
	int iImgPosY = ((float)point.y / (float)rc.Height()) * m_iVideoHeight;

	VLK_TrackTargetPositionEx(&param, iImgPosX, iImgPosY, m_iVideoWidth, m_iVideoHeight);

	CStatic::OnLButtonDblClk(nFlags, point);
}

BOOL CVideoDisplayWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (zDelta < 0) // toward the user
	{
		VLK_ZoomOut(4);
		SetTimer(STOP_ZOOMING_TIMER_ID, 500, NULL);
		
	}
	else if (zDelta > 0) // away from the user
	{
		VLK_ZoomIn(4);
		SetTimer(STOP_ZOOMING_TIMER_ID, 500, NULL);
	}

	return CStatic::OnMouseWheel(nFlags, zDelta, pt);
}


void CVideoDisplayWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	VLK_StopTrack();

	CStatic::OnRButtonDown(nFlags, point);
}


void CVideoDisplayWnd::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (STOP_ZOOMING_TIMER_ID == nIDEvent)
	{
		KillTimer(STOP_ZOOMING_TIMER_ID);
		VLK_StopZoom();
	}

	CStatic::OnTimer(nIDEvent);
}


void CVideoDisplayWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 CStatic::OnPaint()
	CRect rc;
	GetClientRect(rc);

	dc.FillSolidRect(rc, RGB(0, 0, 0));
}


void CVideoDisplayWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	SetFocus();

	CStatic::OnLButtonDown(nFlags, point);
}

LRESULT CVideoDisplayWnd::OnVideoResolution(WPARAM wParam, LPARAM lParam)
{
	m_iVideoWidth = wParam;
	m_iVideoHeight = lParam;
	return 0;
}
