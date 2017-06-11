#include "Node2D.h"

#define SafeRelease(p) if(p){p->Release(); p=NULL;}

inline Node2DView* TransNode2DView(CGDIView* pView)
{
	return (Node2DView*)pView;
}

CNode2D::CNode2D(CNode* parent):
	CNode(parent)
{
	assert(parent);
}

CNode2D::~CNode2D()
{
}

void CNode2D::DrawNode(DrawKit* pDrawKit)
{
}

void CNode2D::CalculateRect()
{
	if (m_bNeedUpdateRect)
	{
		m_bNeedUpdateRect = false;

		m_rectF.X = -m_pairAnchor.first*m_pairSize.first;
		m_rectF.Y = -m_pairAnchor.second*m_pairSize.second;
		m_rectF.Width = m_pairSize.first;
		m_rectF.Height = m_pairSize.second;

		m_rectD2D1.left = m_rectF.X;
		m_rectD2D1.top = m_rectF.Y;
		m_rectD2D1.right = m_rectF.X + m_rectF.Width;
		m_rectD2D1.bottom = m_rectF.Y + m_rectF.Height;
		if (m_pParent)
		{
			RECT r = m_pParent->GetRect();
			float fx = r.left + m_pairPos.first;
			float fy = r.top + m_pairPos.second;
			fx += m_rectF.X;
			fy += m_rectF.Y;

			m_rect.left = static_cast<int>(fx + 0.5f);
			m_rect.top = static_cast<int>(fy + 0.5f);
			m_rect.right = static_cast<int>(fx + m_pairSize.first + 0.5f);
			m_rect.bottom = static_cast<int>(fy + m_pairSize.second + 0.5f);
		}
	}
}

const D2D_RECT_F& CNode2D::GetRectD2D1()
{
	CalculateRect();
	return m_rectD2D1;
}

void CScene2D::DrawNode(DrawKit* pDrawKit)
{
	if (m_bNeedSortChild)
	{
		SortChild();
	}

	CNode2D* pNode;
	D2D1::Matrix3x2F matTrans, matScale, matRotate;
	auto pTar = ((Node2DView*)(pDrawKit->pView))->GetRenderTarget();

	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
	{
		pNode = (CNode2D*)*iter;
		if (pNode->IsVisible())
		{
			auto& pos = pNode->GetPos();
			auto& scale = pNode->GetScale();
			auto rotate = pNode->GetRotate();
			matTrans = D2D1::Matrix3x2F::Translation(pos.first, pos.second);
			matScale = D2D1::Matrix3x2F::Scale(scale.first, scale.second);
			matRotate = D2D1::Matrix3x2F::Rotation(rotate);
			pTar->SetTransform(matTrans*matScale*matRotate);
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
		GetRectD2D1(),
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
	assert(GetView() != NULL);

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
	auto pView = TransNode2DView(GetView());
	pView->GetRenderTarget()->DrawBitmap(m_pBitmap,
		GetRectD2D1(),
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


bool CTestScene2D::Init()
{
	EnableCustomNCHitTest(false);

	CNode2DTextLayer* pText = new CNode2DTextLayer(this);
	pText->CreateTextFormat(L"Î¢ÈíÑÅºÚ", 18);
	pText->SetText(L"È·¶¨");
	pText->SetTextColor(D2D1::ColorF::Black);
	pText->SetAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pText->SetSize(100, 100);
	pText->SetPos(GetSize().first / 2, GetSize().second / 2);

	CNode2DImageLayer* pImage = new CNode2DImageLayer(this);
	pImage->CreateImageLayerByColor(0, 122, 204, 225);
	pImage->SetSize(100, 100);
	pImage->SetPos(50, 50);

	return CScene2D::Init();
}

void CTestScene2D::DrawNode(DrawKit* pDrawKit)
{
//	auto pView = (Node2DView*)(pDrawKit->pView);
//	auto t = D2D1::Matrix3x2F::Translation(25, 25);
//	auto r = D2D1::Matrix3x2F::Rotation(45.0f, D2D1::Point2F(0, 0));
//	auto s = D2D1::Matrix3x2F::Scale(0.5f, 2.0f, D2D1::Point2F(0, 40));
//	pView->GetRenderTarget()->SetTransform(t*r);
//	pView->GetRenderTarget()->GetTransform(&r);
	//pView->GetRenderTarget()->SetTransform(r*t);
	return CScene2D::DrawNode(pDrawKit);

	//ID2D1SolidColorBrush * pBrush;
	//pView->GetRenderTarget()->CreateSolidColorBrush(D2D1::ColorF(RGB(255, 128, 64)), &pBrush);
	//pView->GetRenderTarget()->DrawLine(D2D1::Point2F(0, 0), D2D1::Point2F(105, 105), pBrush);
	//pBrush->Release();
}