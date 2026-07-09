
// demo_for_windows.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// Cdemo_for_windowsApp: 
// 有关此类的实现，请参阅 demo_for_windows.cpp
//

class Cdemo_for_windowsApp : public CWinApp
{
public:
	Cdemo_for_windowsApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};

extern Cdemo_for_windowsApp theApp;