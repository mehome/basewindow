#include "View.h"
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")

GdiplusInit& GdiplusInit::Instance()
{
	static GdiplusInit instance;
	return instance;
}

GdiplusInit::GdiplusInit()
{
	Gdiplus::GdiplusStartupInput input;
	Gdiplus::GdiplusStartup(&m_token, &input, NULL);
}
GdiplusInit::~GdiplusInit()
{
	Gdiplus::GdiplusShutdown(m_token);
}

HDC CGDIView::GetScreenDC()
{
	static std::unique_ptr<void, win_handle_deleter<2>> hScreenDC;

	if (!hScreenDC)
	{
		hScreenDC.reset(GetDC(NULL));
	}

	return (HDC)hScreenDC.get();
}

CGDIView::CGDIView() :m_Wnd(NULL),
	m_DC(NULL),
	m_MemDC(NULL),
	m_MemBitmap(NULL),
	m_bRefresh_(false)
{
}

CGDIView::~CGDIView()
{
}

bool CGDIView::Init(HWND hWnd)
{
	assert(IsWindow(hWnd));
	//GdiplusInit::Instance();
	timeBeginPeriod(1);

	GetClientRect(hWnd, &m_rectWnd);
	GetClientRect(GetDesktopWindow(), &m_rectScreen);
	m_Wnd = hWnd;
	m_DC = GetDC(hWnd);
	if (!m_DC)
	{
		return false;
	}
	m_MemDC = CreateCompatibleDC(m_DC);
	if (!m_MemDC)
	{
		ReleaseDC(m_Wnd, m_DC);
		return false;
	}
	//m_MemBitmap = CreateCompatibleBitmap(m_DC, m_rectScreen.right - m_rectScreen.left, m_rectScreen.bottom - m_rectScreen.top);
	memset(&bitmapHeader_, 0, sizeof(bitmapHeader_));
	bitmapHeader_.biSize = sizeof(BITMAPINFOHEADER);
	bitmapHeader_.biPlanes = 1;
	bitmapHeader_.biHeight = m_rectScreen.bottom - m_rectScreen.top;
	bitmapHeader_.biWidth = m_rectScreen.right - m_rectScreen.left;
	bitmapHeader_.biSizeImage = bitmapHeader_.biHeight*bitmapHeader_.biWidth * 4;
	bitmapHeader_.biBitCount = 32;
	bitmapHeader_.biCompression = BI_RGB;
	m_MemBitmap = CreateDIBSection(m_DC, (BITMAPINFO*)&bitmapHeader_, DIB_RGB_COLORS, (void**)&pBmpData_, NULL, 0);
	if (!m_MemBitmap)
	{
		DeleteDC(m_MemDC);
		ReleaseDC(m_Wnd, m_DC);
		return false;
	}
	SelectObject(m_MemDC, m_MemBitmap);
	SetBkMode(m_MemDC, TRANSPARENT);
	SetStretchBltMode(m_MemDC, STRETCH_HALFTONE);
	SetBrushOrgEx(m_MemDC, 0, 0, NULL);
	SetGraphicsMode(m_MemDC, GM_ADVANCED);
	//m_pGraphics = Gdiplus::Graphics::FromHDC(m_MemDC);

	return true;
}

void CGDIView::Clear()
{
	//if (m_pGraphics)
	//{
	//	delete m_pGraphics;
	//	m_pGraphics = NULL;
	//}
	if (m_MemBitmap)
	{
		DeleteObject(m_MemBitmap);
		m_MemBitmap = NULL;
	}
	if (m_MemDC)
	{
		DeleteDC(m_MemDC);
		m_MemDC = NULL;
	}
	if (m_DC)
	{
		ReleaseDC(m_Wnd, m_DC);
		m_DC = NULL;
	}

	m_Wnd = NULL;
	timeEndPeriod(1);
}

void CGDIView::ClearBuffer()
{
	FillRect(m_MemDC, &m_rectScreen, (HBRUSH)GetStockObject(WHITE_BRUSH));
}

void CGDIView::SwapBuffer()
{
	BitBlt(m_DC, 0, 0, m_rectScreen.right - m_rectScreen.left, m_rectScreen.bottom - m_rectScreen.top, m_MemDC, 0, 0, SRCCOPY);
}

void CGDIView::Refresh()
{
	m_bRefresh_ = true;
}

void CGDIView::DoRefresh()
{
	if (m_bRefresh_)
	{
		m_bRefresh_ = false;
		InvalidateRect(m_Wnd, NULL, FALSE);
		//PostMessage(m_Wnd, WM_PAINT, 0, 0);
	}
}

void CGDIView::WndRectChanged()
{
	GetClientRect(m_Wnd, &m_rectWnd);
}

CGDIViewAlpha::CGDIViewAlpha()
{
	blendFunc_.SourceConstantAlpha = 255;
	blendFunc_.BlendFlags = 0;
	blendFunc_.BlendOp = AC_SRC_OVER;
	blendFunc_.AlphaFormat = AC_SRC_ALPHA;
}

bool CGDIViewAlpha::Init(HWND hWnd)
{
	if (!CGDIView::Init(hWnd))
	{
		return false;
	}

	LONG_PTR style = GetWindowLong(GetWnd(), GWL_EXSTYLE);
	if (!(style & WS_EX_LAYERED))
	{
		style |= WS_EX_LAYERED;
		SetWindowLong(GetWnd(), GWL_EXSTYLE, style);
		PostMessage(GetWnd(), WM_PAINT, 0, 0);
	}

	return true;
}

void CGDIViewAlpha::ClearBuffer()
{
	int w, h;

	w = m_rectScreen.right - m_rectScreen.left;
	h = m_rectScreen.bottom - m_rectScreen.top;
	memset(pBmpData_, 0, w * h * 4);
}

void CGDIViewAlpha::SwapBuffer()
{
	//SolidifyGDIPixple();

	POINT pos;
	SIZE size;

	pos.x = pos.y = 0;
	size.cx = m_rectWnd.right - m_rectWnd.left;
	size.cy = m_rectWnd.bottom - m_rectWnd.top;

	UpdateLayeredWindow(m_Wnd, m_DC, NULL, &size, m_MemDC, &pos, RGB(255, 255, 255), &blendFunc_, ULW_ALPHA);
}

void CGDIViewAlpha::WndRectChanged()
{
	RECT rect;
	GetClientRect(m_Wnd, &rect);
	if(!EqualRect(&rect, &m_rectWnd))
	{
		m_rectWnd=rect;
		PostMessage(GetWnd(), WM_PAINT, 0, 0);
	}
}

void CGDIViewAlpha::SolidifyGDIPixple()
{
	unsigned char *p1, *p2;
	int j, i;
	int w, h, sw, sh;

	sw = m_rectScreen.right - m_rectScreen.left;
	sh = m_rectScreen.bottom - m_rectScreen.top;
	w = m_rectWnd.right - m_rectWnd.left;
	h = m_rectWnd.bottom - m_rectWnd.top;
	for (j = 0; j < h; ++j)
	{
		p1 = pBmpData_ + (sh-1-j) * sw * 4;
		for (i = 0; i < w; ++i)
		{
			p2 = p1 + i * 4;
			if ((p2[3] == 0) && (p2[0] != 0 || p2[1] != 0 || p2[2] != 0))
				p2[3] = 255;
		}
	}
}