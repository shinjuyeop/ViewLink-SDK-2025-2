#include "stdafx.h"
#include "PTZButton.h"


CPTZButton::CPTZButton()
{
}


CPTZButton::~CPTZButton()
{
}
BEGIN_MESSAGE_MAP(CPTZButton, CButton)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


void CPTZButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	::SendMessage(GetParent()->m_hWnd, WM_BUTTON_PRESSED, (WPARAM)this, (LPARAM)0);

	CButton::OnLButtonDown(nFlags, point);
}


void CPTZButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	::SendMessage(GetParent()->m_hWnd, WM_BUTTON_RELEASED, (WPARAM)this, (LPARAM)0);

	CButton::OnLButtonUp(nFlags, point);
}
