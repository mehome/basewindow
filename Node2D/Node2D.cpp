#include "Node2D.h"

#define SafeRelease(p) if(p){p->Release(); p=NULL;}

inline Node2DView* TransNode2DView(CGDIView* pView)
{
	return (Node2DView*)pView;
}

CNode2D::CNode2D(CNode* parent):
	CNode(parent)
{
}

CNode2D::~CNode2D()
{
}

void CNode2D::DrawNode(DrawKit* pDrawKit)
{
}

void CScene2D::DrawNode(DrawKit* pDrawKit)
{
	if (m_bNeedSortChild)
	{
		SortChild();
	}

	CNode2D* pNode;
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
	{
		pNode = (CNode2D*)*iter;
		if (pNode->IsVisible())
		{
			pDrawKit->pParent = this;
			pNode->DrawNode(pDrawKit);
		}
	}
}

CNode2DTextLayer::CNode2DTextLayer(CNode* parent):
	CNode2D(parent),
	m_pTextColor(NULL),
	m_pTextFormat(NULL)
{
}

CNode2DTextLayer::~CNode2DTextLayer()
{
	SafeRelease(m_pTextColor);
	SafeRelease(m_pTextFormat);
}

void CNode2DTextLayer::DrawNode(DrawKit* pDrawKit)
{
	auto pView = TransNode2DView(pDrawKit->pView);

	pView->GetRenderTarget()->DrawText(m_strText.c_str(),
		m_strText.length(),
		m_pTextFormat,
		D2D1::RectF(0, 0, 100, 100),
		m_pTextColor);
}

void CNode2DTextLayer::CreateTextFormat(const std::wstring strFontName, float fontSize)
{
	HRESULT hr;
	SafeRelease(m_pTextFormat);
	hr = NodeDWrite::Instance().GetDWrireFactory()->CreateTextFormat(
		strFontName.c_str(),
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		fontSize,
		L"", //locale
		&m_pTextFormat
	);
	if (FAILED(hr))
	{
		TRACE("cannt CreateTextFormat");
	}
}

void CNode2DTextLayer::SetAlignment(DWRITE_TEXT_ALIGNMENT align)
{
	if (m_pTextFormat)
	{
		m_pTextFormat->SetTextAlignment(align);
	}
}

void CNode2DTextLayer::SetTextColor(D2D1::ColorF c)
{
	if (!GetView())
	{
		TRACE("cannt get render view");
		return;
	}

	SafeRelease(m_pTextColor);
	auto pView = TransNode2DView(GetView());
	auto hr = pView->GetRenderTarget()->CreateSolidColorBrush(c, &m_pTextColor);
	if (FAILED(hr))
	{
		throw std::exception("failed CreateSolidColorBrush");
	}
}

void CNode2DTextLayer::SetText(const std::wstring& text)
{
	m_strText = text;
}

CNode2DTextLayerLayout::CNode2DTextLayerLayout(CNode* parent):
	CNode2DTextLayer(parent),
	m_pTextLayout(0)
{
}

CNode2DTextLayerLayout::~CNode2DTextLayerLayout()
{
	SafeRelease(m_pTextLayout);
}

bool CNode2DTextLayerLayout::CreateTextLayout()
{
	if (!m_pTextFormat || m_strText.empty())
	{
		return false;
	}
	SafeRelease(m_pTextLayout);
	auto hr = NodeDWrite::Instance().GetDWrireFactory()->CreateTextLayout(m_strText.c_str(),
		m_strText.length(), m_pTextFormat, 100, 100, &m_pTextLayout);

	DWRITE_TEXT_METRICS me;
	m_pTextLayout->GetMetrics(&me);
	m_pTextLayout->SetMaxHeight(me.height);
	m_pTextLayout->SetMaxWidth(me.width);

	return SUCCEEDED(hr);
}

void CNode2DTextLayerLayout::SetText(const std::wstring& text)
{
	m_strText = text;
	CreateTextLayout();
}

void CNode2DTextLayerLayout::DrawNode(DrawKit* pDrawKit)
{
	auto pView = TransNode2DView(pDrawKit->pView);
	D2D1_POINT_2F pos;
	pos.x = 0;
	pos.y = 0;
	pView->GetRenderTarget()->DrawTextLayout(pos, m_pTextLayout, m_pTextColor);
}