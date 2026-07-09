#pragma once
#include "afxwin.h"
class CVideoDisplayWnd : public CStatic
{
public:
	CVideoDisplayWnd();
	virtual ~CVideoDisplayWnd();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT OnVideoResolution(WPARAM wParam, LPARAM lParam);
private:
	int m_iVideoWidth;
	int m_iVideoHeight;
};

