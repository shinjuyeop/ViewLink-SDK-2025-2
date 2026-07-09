#include "stdafx.h"
#include "DirectDrawObj.h"
#pragma comment(lib, "legacy_stdio_definitions.lib")

#define IID_IDirectDraw7 DEC_IID_IDirectDraw7
static const GUID DEC_IID_IDirectDraw7 = {0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b};

CDirectDrawObj::CDirectDrawObj(void)
	: m_pDDraw(NULL), 
	m_pMainSurface(NULL), 
	m_pSubSurface(NULL), 
	m_hWnd(NULL),
	m_pDClipper(NULL)
{
}

CDirectDrawObj::~CDirectDrawObj(void)
{
	Close();
}

void CDirectDrawObj::Close()
{
	Reset();

	m_mutex.lock();

	if (m_pDClipper)
	{
		m_pDClipper->Release();
		m_pDClipper = NULL;
	}

	if (m_pMainSurface)
	{
		IDirectDrawSurface7_Release(m_pMainSurface);
		m_pMainSurface = NULL;
	}

	if (m_pSubSurface)
	{
		IDirectDrawSurface7_Release(m_pSubSurface);
		m_pSubSurface = NULL;
	}

	if(m_pDDraw)
	{
		IDirectDraw7_Release(m_pDDraw);
		m_pDDraw = NULL;
	}
	m_mutex.unlock();
}

long CDirectDrawObj::Open(const GUID* pGuId, HWND hWnd, long lWidth, long lHeight, AVPixelFormat pix_fmt)
{
	if(lWidth <= 0 || lHeight <= 0)
	{
		TRACE("display card input param error[%d, %d]\n", lWidth, lHeight);
		return -1;
	}

	Close();
	m_hWnd = hWnd;

	do
	{
		HRESULT hr = ::DirectDrawCreateEx((GUID*)pGuId, (LPVOID*)&m_pDDraw, IID_IDirectDraw7, NULL);
		if(hr != DD_OK)
		{
			TRACE("display card create draw error[%0x]\n", hr);
			break;
		}

// 		DDCAPS ddcaps;
// 		ddcaps.dwSize = sizeof(ddcaps);
// 		if (SUCCEEDED(m_pDDraw->GetCaps(&ddcaps, NULL)))
// 		{
// 
// 		}

		hr = m_pDDraw->SetCooperativeLevel(NULL, DDSCL_NORMAL);
		if(hr != DD_OK)
		{
			TRACE("display card set level error[%0x]\n", hr);
			break;
		}

 		//DWORD dwFrequency = 60;
 		//m_pDDraw->GetMonitorFrequency(&dwFrequency);
 		//::JLogFile("display card refresh=%d", dwFrequency);
 		//hr = m_pDDraw->SetDisplayMode(lWidth, lHeight, m_dwBPP, dwFrequency, 0);
 		//if(hr != DD_OK)
 		//{
 		//	JLogFile("display card set display error[%0x]", hr);
 		//}

		m_pMainSurface = DDCreateSurface(DDSD_CAPS, DDSCAPS_PRIMARYSURFACE);
		if(NULL == m_pMainSurface)
		{
			TRACE("display card create main surface error\n");
			break;
		}
		m_pSubSurface = DDCreateSurface(DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT , DDSCAPS_OFFSCREENPLAIN/* | DDSCAPS_SYSTEMMEMORY*/, lWidth, lHeight, pix_fmt);
		if(NULL == m_pSubSurface)
		{
			TRACE("display card create sub surface error\n");
			break;
		}

		m_pDClipper = DDCreateClipper(m_pMainSurface, hWnd);
		return 0;
	}while(0);
	
	Close();
	return -2;
}

IDirectDrawSurface7* CDirectDrawObj::DDCreateSurface(DWORD dwFlags, DWORD dwCaps, long lWidth, long lHeight, AVPixelFormat pix_fmt)
{
	if(NULL == m_pDDraw)
	{
		return NULL;
	}

	m_mutex.lock();
	IDirectDrawSurface7* pSurface = NULL;
	DDSURFACEDESC2 stSurfaceDesc;
	memset(&stSurfaceDesc, 0, sizeof(stSurfaceDesc));
	stSurfaceDesc.dwSize = sizeof(stSurfaceDesc);
	stSurfaceDesc.dwFlags = dwFlags;
	stSurfaceDesc.ddsCaps.dwCaps = dwCaps;
	stSurfaceDesc.dwWidth = lWidth;
	stSurfaceDesc.dwHeight = lHeight;
	if(lWidth > 0 && lHeight > 0)
	{
		stSurfaceDesc.dwFlags |= DDSD_PIXELFORMAT;
		stSurfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		if(pix_fmt == AV_PIX_FMT_YUV420P || pix_fmt == AV_PIX_FMT_YUVJ420P)
		{
			stSurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
			stSurfaceDesc.ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y', 'V', '1', '2');
			stSurfaceDesc.ddpfPixelFormat.dwYUVBitCount = 12;
		}
		else if (pix_fmt == AV_PIX_FMT_RGB32)
		{
			stSurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
			stSurfaceDesc.ddpfPixelFormat.dwRGBBitCount = 32;
			stSurfaceDesc.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
			stSurfaceDesc.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
			stSurfaceDesc.ddpfPixelFormat.dwBBitMask = 0x000000FF;
		}

	}
	HRESULT hr = m_pDDraw->CreateSurface(&stSurfaceDesc, &pSurface, NULL);
	if(hr != DD_OK)
	{
		TRACE("display card create surface error[%0x]: %s, width=%d, height=%d\n", hr, "hr", lWidth, lHeight);
		m_mutex.unlock();
		return NULL;
	}

	if(lWidth > 0 && lHeight > 0)
	{
		DDSURFACEDESC2 stLockDesc;
		memset(&stLockDesc, 0, sizeof(stLockDesc));
		stLockDesc.dwSize = sizeof(stLockDesc);
		while(DD_OK != (hr = IDirectDrawSurface_Lock(pSurface, NULL, &stLockDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL)))
		{
			if(DDERR_SURFACELOST == hr || DD_OK != IDirectDrawSurface_Restore(pSurface))
			{
				IDirectDrawSurface_Release(pSurface);
				TRACE("display card lock surface error[%0x]\n", hr);
				m_mutex.unlock();
				return NULL;
			}
		}
		memset(stLockDesc.lpSurface, 0, stLockDesc.lPitch * stLockDesc.dwHeight);
		IDirectDrawSurface_Unlock(pSurface, NULL);
	}

	m_mutex.unlock();
	return pSurface;
}

long CDirectDrawObj::Input(const AVFrame *frame)
{
	m_mutex.lock();

	//主表面丢失
	if(IDirectDrawSurface_IsLost(m_pMainSurface))
	{
		IDirectDrawSurface_Restore(m_pMainSurface);
	}

	//写入离表面数据
	HRESULT hr = DD_OK;
	DDSURFACEDESC2 stSurfaceDesc;
	memset(&stSurfaceDesc, 0, sizeof(stSurfaceDesc));
	stSurfaceDesc.dwSize = sizeof(stSurfaceDesc);
	while(DD_OK != (hr = IDirectDrawSurface_Lock(m_pSubSurface, NULL, &stSurfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL)))
	{
		if(DDERR_SURFACELOST == hr || DD_OK != IDirectDrawSurface_Restore(m_pSubSurface))
		{
			m_mutex.unlock();
			return -1;
		}
	}

	byte * pDest = (BYTE *)stSurfaceDesc.lpSurface;
	int stride = stSurfaceDesc.lPitch;
	memcpy(pDest, frame->data[0], frame->linesize[0] * frame->height);
	pDest += (frame->linesize[0] * frame->height);

	memcpy(pDest, frame->data[2], frame->linesize[2] * (frame->height>>1));
	pDest += (frame->linesize[2] * (frame->height >> 1));

	memcpy(pDest, frame->data[1], frame->linesize[1] * (frame->height >> 1));


	IDirectDrawSurface_Unlock(m_pSubSurface, NULL);

	//递交主表面
	DDBLTFX stBltFX = {0};
	stBltFX.dwSize = sizeof(DDBLTFX);
	stBltFX.dwDDFX = DDBLTFX_NOTEARING;
	RECT rc;
	::GetWindowRect(m_hWnd, &rc);
	hr = m_pMainSurface->Blt(&rc, m_pSubSurface, NULL, DDBLT_WAIT, &stBltFX);
	m_mutex.unlock();
	return hr == DD_OK ? 0 : -2;
}

IDirectDrawClipper* CDirectDrawObj::DDCreateClipper(IDirectDrawSurface7* pSurface, HWND hWnd)
{
	IDirectDrawClipper* pDClipper = NULL;
	HRESULT hr = m_pDDraw->CreateClipper(0, &pDClipper, NULL);
	if (DD_OK == hr)
	{
		hr = pDClipper->SetHWnd(0, hWnd);
		if (DD_OK == hr)
		{
			hr = pSurface->SetClipper(pDClipper);
		}
	}
	return pDClipper;
}

void CDirectDrawObj::Reset()
{
	if(NULL == m_pDDraw || NULL == m_pMainSurface || NULL == m_pSubSurface)
	{
		return;
	}

	m_mutex.lock();
	HRESULT hr = DD_OK;
	DDSURFACEDESC2 stSurfaceDesc;
	memset(&stSurfaceDesc, 0, sizeof(stSurfaceDesc));
	stSurfaceDesc.dwSize = sizeof(stSurfaceDesc);
	while(DD_OK != (hr = IDirectDrawSurface_Lock(m_pSubSurface, NULL, &stSurfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL)))
	{
		if(DDERR_SURFACELOST == hr || DD_OK != IDirectDrawSurface_Restore(m_pSubSurface))
		{
			m_mutex.unlock();
			return;
		}
	}
	memset(stSurfaceDesc.lpSurface, 0, stSurfaceDesc.lPitch * stSurfaceDesc.dwHeight);
	IDirectDrawSurface_Unlock(m_pSubSurface, NULL);
	hr = m_pMainSurface->Blt(NULL, m_pSubSurface, NULL, DDBLT_WAIT, NULL);
	m_mutex.unlock();
}