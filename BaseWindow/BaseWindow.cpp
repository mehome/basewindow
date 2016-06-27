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
	UINT flag= SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOCOPYBITS;

	if(w <1 || h <1)
	{
		w=h=0;
		flag |= SWP_NOSIZE;
	}
	
	if(bCenterWindow)
	{
		auto parent=GetParent(m_hWnd);
		if(!parent) parent=GetWindow(m_hWnd, GW_OWNER);
		if(parent)
		{
			RECT r;
			GetClientRect(parent, &r);
			posx=(r.right-r.left - w)/2;
			posy=(r.bottom-r.top - h)/2;
		}
		else
		{
			posx=(GetSystemMetrics(SM_CXSCREEN) - w) /2;
			posy=(GetSystemMetrics(SM_CYSCREEN) - h) /2;
		}
	}
	else
	{
		posx=0;
		posy=0;
		flag |= SWP_NOMOVE;
	}

	SetWindowPos(m_hWnd, NULL, posx, posy, w, h, flag);
}

void CBaseWindow::InitWindow(HINSTANCE hInstance,
	HWND hParent,
	bool bAsChildOrOwned)
{
	static std::wstring szClassName=L"base_window42";
	WNDCLASSEX wcex;
	auto flag = WS_OVERLAPPEDWINDOW;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_IME;
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

void CBaseWindow::RunWindow()
{
	if(!m_hWnd)
	{
		return;
	}

	SetWindowLong(m_hWnd, GWL_USERDATA, (LONG)this);
	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);

	MSG msg;
	BOOL bRet;
	while((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if(bRet == -1)
		{
			TRACE1("GetMessage return -1:%u\n",GetLastError());
			break;
		}
		if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
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
			PAINTSTRUCT ps;
			RECT rect;
			HDC hDC=BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rect);
			FillRect(hDC, &rect, (HBRUSH)GetStockObject(DKGRAY_BRUSH));
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_CREATE:
		CBaseWindow* p1=new CBaseWindow();
		p1->InitWindow(GetModuleHandle(NULL), hWnd, false);
		p1->ReSize(200, 150, true);
		break;
	}
	return CBaseWindow::CustomProc(hWnd, message, wParam, lParam, bProcessed);
}
