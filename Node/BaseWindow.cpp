#include "BaseWindow.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CBaseWindow* pThis;
	bool bProcessed(false);
	LRESULT res;

	if(message == WM_CREATE)
	{
		pThis=(CBaseWindow*)((CREATESTRUCT*)lParam)->lpCreateParams;
		pThis->m_hWnd=hWnd;
	}
	else
	{
		pThis=(CBaseWindow*)(GetWindowLong(hWnd, GWL_USERDATA));
	}

	if(pThis)
	{
		res=pThis->CustomProc(hWnd, message, wParam, lParam, bProcessed);
		if(bProcessed)
		{
			return res;
		}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

RECT CBaseWindow::GetAvaliableDestktopArea()
{
	RECT area = { 0 };
	HWND hwnd = FindWindow(L"Shell_TrayWnd", L"");
	RECT rect;
	GetWindowRect(hwnd, &rect);
	if (rect.top != 0)
	{
		// 任务栏在下方
		area.left = 0;
		area.right = GetSystemMetrics(SM_CXSCREEN);
		area.top = 0;
		area.bottom = GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top);
	}
	else
	{
		if (rect.left != 0)
		{
			// 右方
			area.top = 0;
			area.bottom = GetSystemMetrics(SM_CYSCREEN);
			area.left = 0;
			area.right = GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left);
		}
		else if (rect.bottom != GetSystemMetrics(SM_CYSCREEN))
		{
			// 上方
			area.left = 0;
			area.right = GetSystemMetrics(SM_CXSCREEN);
			area.top = rect.bottom - rect.top;
			area.bottom = GetSystemMetrics(SM_CYSCREEN);
		}
		else
		{
			//左方
			area.top = 0;
			area.bottom = GetSystemMetrics(SM_CYSCREEN);
			area.left = rect.right - rect.left;
			area.right = GetSystemMetrics(SM_CXSCREEN);
		}
	}

	return area;
}


CBaseWindow::CBaseWindow(void):m_hWnd(NULL)
{
}

CBaseWindow::~CBaseWindow(void)
{
}

HWND CBaseWindow::GetHWND()
{
	return m_hWnd;
}

void CBaseWindow::ReSize(int w,int h, bool bCenterWindow)
{
	if(!m_hWnd)
		return;

	int posx,posy;
	UINT flag= SWP_NOZORDER ;//| SWP_NOOWNERZORDER | SWP_NOCOPYBITS;

	if(w <1 || h <1)
	{
		w=h=0;
		flag |= SWP_NOSIZE;
	}
	
	if(bCenterWindow)
	{
		auto parent=GetParent(m_hWnd);
		auto onwer=GetWindow(m_hWnd, GW_OWNER);
		if(parent)
		{
			RECT r;
			GetClientRect(parent, &r);
			posx=(r.right-r.left - w)/2;
			posy=(r.bottom-r.top - h)/2;
		}
		else
		{
			RECT area = GetAvaliableDestktopArea();
			posx=area.left + ((area.right-area.left) - w) /2;
			posy=area.top + ((area.bottom-area.top) - h) /2;
		}
	}
	else
	{
		posx=0;
		posy=0;
		flag |= SWP_NOMOVE;
	}

	auto res=SetWindowPos(m_hWnd, NULL, posx, posy, w, h, flag);
}

void CBaseWindow::InitWindow(HINSTANCE hInstance,
	HWND hParent,
	bool bAsChildOrOwned)
{
	static std::wstring szClassName=L"base_window42";
	WNDCLASSEX wcex;
	auto flag = WS_OVERLAPPEDWINDOW;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)COLOR_WINDOW;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szClassName.c_str();
	wcex.hIconSm		= NULL;

	if(!RegisterClassEx(&wcex) && 1410 != GetLastError()) // 1410类别已存在
	{
		TRACE1("failed register window class:%u\n", GetLastError());
		return;
	}

	if(hParent)
	{
		flag= (bAsChildOrOwned ? WS_CHILD : 0) | WS_VISIBLE | WS_SYSMENU | WS_THICKFRAME | WS_CAPTION | WS_OVERLAPPED;
	}

	m_hWnd=CreateWindowEx(0,
		szClassName.c_str(),
		L"BaseWindow",
		flag,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hParent,
		NULL,
		hInstance,
		this);

	if(!m_hWnd)
	{
		TRACE1("failed createwindow:%u\n", GetLastError());
	}
}

void CBaseWindow::Show()
{
	if(!m_hWnd)
	{
		return;
	}

	SetWindowLong(m_hWnd, GWL_USERDATA, (LONG)this);
	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);
}

LRESULT CBaseWindow::CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed)
{
	switch(message)
	{
	case WM_DESTROY:
		TRACE("WM_DESTROY\n");
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		TRACE("WM_CREATE\n");
		break;
	}
	
	bProcessed=false;
	return 0;
}

LRESULT CTestSubControl::CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed)
{
	switch(message)
	{
	case WM_PAINT:
		{
			TRACE("WM_PAINT\n");
			PAINTSTRUCT ps;
			RECT rect;
			HDC hDC=BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rect);
			FillRect(hDC, &rect, (HBRUSH)GetStockObject(DKGRAY_BRUSH));
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_CREATE:
		{
			CBaseWindow* p1=new CBaseWindow();
			p1->InitWindow(GetModuleHandle(NULL), hWnd);
			p1->ReSize(200, 150, true);
		}
		break;
	case WM_SIZE:
		{
			TRACE("WM_SIZE\n");}
		break;
	}
	return CBaseWindow::CustomProc(hWnd, message, wParam, lParam, bProcessed);
}
