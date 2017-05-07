#include "Node2DView.h"

#define SafeRelease(p) if(p){p->Release(); p=NULL;}

NodeDWrite::NodeDWrite():
	m_pDWriteFactory(NULL)
{
	HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory),
		reinterpret_cast<IUnknown **>(&m_pDWriteFactory));
	if (FAILED(hr))
	{
		TRACE1("failed DWriteCreateFactory %d", (int)hr);
		throw std::exception("failed DWriteCreateFactory");
	}
}

NodeDWrite::~NodeDWrite()
{
	SafeRelease(m_pDWriteFactory);
}

NodeDWrite& NodeDWrite::Instance()
{
	static NodeDWrite static_global_instance;
	return static_global_instance;
}

Node2DView::Node2DView():
	m_pD2DFactory(NULL),
	m_pRenderTarget(NULL),
	m_pDWrite(NULL)
{
}

Node2DView::~Node2DView()
{
}

bool Node2DView::Init(HWND hWnd)
{
	HRESULT hr;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	if (FAILED(hr))
	{
		return false;
	}

	m_Wnd = hWnd;
	GetClientRect(hWnd, &m_rectWnd);
	D2D1_SIZE_U size = D2D1::SizeU(
		m_rectWnd.right - m_rectWnd.left,
		m_rectWnd.bottom - m_rectWnd.top
	);
	hr = m_pD2DFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(m_Wnd, size),
		&m_pRenderTarget
	);
	if (FAILED(hr))
	{
		return false;
	}

	m_pDWrite = &NodeDWrite::Instance();

	return true;
}

void Node2DView::Clear()
{
	SafeRelease(m_pRenderTarget);
	SafeRelease(m_pD2DFactory);
}

void Node2DView::WndRectChanged()
{
	GetClientRect(m_Wnd, &m_rectWnd);

	if (m_pRenderTarget)
	{
		D2D1_SIZE_U size;
		size.width = m_rectWnd.right - m_rectWnd.left;
		size.height = m_rectWnd.bottom - m_rectWnd.top;
		m_pRenderTarget->Resize(size);
	}
}

void Node2DView::ClearBuffer()
{
	HRESULT hr(S_OK);
	if (!m_pRenderTarget)
	{
		GetClientRect(m_Wnd, &m_rectWnd);
		D2D1_SIZE_U size = D2D1::SizeU(
			m_rectWnd.right - m_rectWnd.left,
			m_rectWnd.bottom - m_rectWnd.top
		);

		hr = m_pD2DFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_Wnd, size),
			&m_pRenderTarget
		);
	}
	if (SUCCEEDED(hr) && !(m_pRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
	{
		m_pRenderTarget->BeginDraw();
		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
	}
	else
	{
		SafeRelease(m_pRenderTarget);
	}
}

void Node2DView::SwapBuffer()
{
	HRESULT hr;
	if (!m_pRenderTarget)
		return;
	hr = m_pRenderTarget->EndDraw();
	if (hr == D2DERR_RECREATE_TARGET)
	{
		hr = S_OK;
		SafeRelease(m_pRenderTarget);
	}
}