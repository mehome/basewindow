#pragma once

#include <cassert>
#include <Windows.h>
#include <GdiPlus.h>
#include "Log.h"

class GdiplusInit
{
public:
	static GdiplusInit& Instance();
	GdiplusInit();
	~GdiplusInit();
protected:
	ULONG_PTR m_token;
};

class CGDIView
{
public:
	CGDIView();
	virtual ~CGDIView();
	virtual bool Init(HWND hWnd);
	virtual void Clear();
	virtual void ClearBuffer();
	virtual void SwapBuffer();
	
	unsigned char* GetBitmapData(){return pBmpData_;}
	HDC GetRealDC()const {return m_DC;}
	HDC GetMemDC()const {return m_MemDC;}
	HWND GetWnd()const {return m_Wnd;}
	RECT GetWndRect()const {return m_rectWnd;}
	RECT GetScreenRect()const {return m_rectScreen;}
	Gdiplus::Graphics& GetGraphics() { return *m_pGraphics; };
	void Refresh();
	void DoRefresh();
	virtual void WndRectChanged();
protected:
	HWND m_Wnd;
	RECT m_rectWnd;
	RECT m_rectScreen;
	HDC m_DC;
	HDC m_MemDC;
	HBITMAP m_MemBitmap;
	unsigned char* pBmpData_;
	BITMAPINFOHEADER bitmapHeader_;
	bool m_bRefresh_;

	Gdiplus::Graphics* m_pGraphics;
};

class CGDIViewAlpha :public CGDIView
{
public:
	CGDIViewAlpha();
	bool Init(HWND hWnd);
	void ClearBuffer();
	void SwapBuffer();
	void WndRectChanged();
	void SolidifyGDIPixple();
protected:
	BLENDFUNCTION blendFunc_;
};