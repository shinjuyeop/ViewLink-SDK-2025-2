#pragma once
#include "VideoObjNetwork.h"
#include <Windows.h>
#include <ddraw.h>


class CDirectDrawObj
{
public:
	CDirectDrawObj(void);
	virtual ~CDirectDrawObj(void);

public:
	virtual long		Open(const GUID* pGuId, HWND hWnd, long lWidth, long lHeight, AVPixelFormat pix_fmt);
	virtual void		Close();
	virtual long		Input(const AVFrame *frame);
protected:
	IDirectDrawSurface7*	DDCreateSurface(DWORD dwFlags, DWORD dwCaps, long lWidth = 0, long lHeight = 0, AVPixelFormat pix_fmt = AV_PIX_FMT_YUVJ420P);
	IDirectDrawClipper* DDCreateClipper(IDirectDrawSurface7* pSurface, HWND hWnd);
	void Reset();
private:
	IDirectDraw7*			m_pDDraw;
	IDirectDrawSurface7*	m_pMainSurface;
	IDirectDrawSurface7*	m_pSubSurface;
	IDirectDrawClipper*		m_pDClipper;

	HWND m_hWnd;	
	std::mutex m_mutex;
};
