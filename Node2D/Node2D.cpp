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
	auto rect = GetRect();

	pView->GetRenderTarget()->DrawText(m_strText.c_str(),
		m_strText.length(),
		m_pTextFormat,
		D2D1::RectF(rect.left, rect.top, rect.right, rect.bottom),
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
	assert(m_pTextFormat);
	if (m_pTextFormat)
	{
		m_pTextFormat->SetTextAlignment(align);
		m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
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

CNode2DImageLayer::CNode2DImageLayer(CNode* parent):
	CNode2D(parent),
	m_pBitmap(NULL)
{
}

CNode2DImageLayer::~CNode2DImageLayer()
{
	SafeRelease(m_pBitmap);
}

void CNode2DImageLayer::DrawNode(DrawKit* pDrawKit)
{
	auto rect = GetRect();
	auto pView = TransNode2DView(GetView());
	pView->GetRenderTarget()->DrawBitmap(m_pBitmap,
		D2D1::RectF(rect.left, rect.top, rect.right, rect.bottom),
		1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
		D2D1::RectF(0, 0, (float)m_sizeImage.width, (float)m_sizeImage.height));
}

bool CNode2DImageLayer::CreateImageLayerByData(unsigned char* pData, int w, int h, int bUseAlpha)
{
	assert(pData && w > 0 && h > 0);
	SafeRelease(m_pBitmap);

	HRESULT hr;
	UINT32 pitch;
	auto pView = TransNode2DView(GetView());
	if (!pView)
		return false;
	pitch = w * 4;
	if (pitch % 4 != 0)
		pitch = pitch / 4 * 4 + 4;
	hr = pView->GetRenderTarget()->CreateBitmap(D2D1::SizeU(w, h),
		pData,
		pitch,
		D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, bUseAlpha ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE)),
		&m_pBitmap);

	bool bRes = SUCCEEDED(hr);
	assert(bRes);
	if (bRes)
	{
		m_sizeImage = m_pBitmap->GetPixelSize();
		auto size = m_pBitmap->GetSize();
		float dx, dy;
		m_pBitmap->GetDpi(&dx, &dy);
		auto pix = m_pBitmap->GetPixelFormat();
	}
	return bRes;
}

bool CNode2DImageLayer::CreateImageLayerByColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	unsigned char data[16 * 16 * 4];
	unsigned char* p;

	for (int i = 0; i < 16 * 16; ++i)
	{
		p = data + i * 4;
		*p = b;
		*(p + 1) = g;
		*(p + 2) = r;
		*(p + 3) = a;
	}

	return CreateImageLayerByData(data, 16, 16, a == 255 ? 0 : 1);
}

const D2D1_SIZE_U& CNode2DImageLayer::GetImageInfoSize()const
{
	return m_sizeImage;
}

void CNode2DImageLayer::UpdateImageData(void* p)
{
	HRESULT hr = m_pBitmap->CopyFromMemory(&D2D1::RectU(0, 0, m_sizeImage.width, m_sizeImage.height), p, m_sizeImage.width * 4);
	assert(SUCCEEDED(hr));
}