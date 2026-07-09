#pragma once
#include "afxwin.h"

#define WM_BUTTON_PRESSED WM_USER + 100
#define WM_BUTTON_RELEASED WM_USER + 101
class CPTZButton : public CButton
{
public:
	CPTZButton();
	virtual ~CPTZButton();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};

