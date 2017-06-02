#include "CutImage.h"
#include "WeatherScene.h"

CircularLayer::CircularLayer(int r, CNode* pParent):CImageLayer(pParent)
{
	assert(r>0 && r <500);
	std::unique_ptr<unsigned char[]> pData(new unsigned char[4*r*r*4]);
	memset(pData.get(), 0, 4*r*r*4);
	CreateImageLayerByData(pData.get(), r*2, r*2, 32);
}

void CircularLayer::CreateCircular(unsigned char opcticy)
{
	auto size = GetSize();
	int w=(int)size.first;
	int h=(int)size.second;
	int r = w/2;
	int x, y;
	unsigned char * pData = ImageData(), *p;

	for (int j = 0; j < h; ++j)
	{
		y = j - r;
		for (int i = 0; i < w; ++i)
		{
			x = i - r;
			if (x*x + y*y > r*r)
			{
				p = pData + j*w * 4 + i * 4;
				*(p + 3) = opcticy;
			}
		}
	}
}

ClipRegion::ClipRegion(int size, CNode* parent, int line_size, COLORREF color) : CNode(parent),
	m_bStretchDown(false),
	n(4),
	m_iStretchIndex(-1),
	m_hCursor(NULL)
{
	SetSize(size, size);
	m_hFramePen = CreatePen(PS_SOLID, line_size, color);
	m_FrameBrush = CreateSolidBrush(color);
	m_FrameDotPen1 = CreatePen(PS_DOT, 1, RGB(200, 200, 200));
	m_FrameDotPen2 = CreatePen(PS_DOT, 1, RGB(160, 160, 160));
}

ClipRegion::~ClipRegion()
{
	if (m_FrameBrush)
		DeleteObject(m_FrameBrush);
	if (m_hFramePen)
		DeleteObject(m_hFramePen);
	if (m_FrameDotPen1)
		DeleteObject(m_FrameDotPen1);
	if (m_FrameDotPen2)
		DeleteObject(m_FrameDotPen2);
}

bool ClipRegion::Init()
{
	m_pShadowLayer=new CImageLayer(this);
	m_pShadowLayer->CreateImageLayerByColor(0, 0, 0, 200);

	m_pCircularLayer=new CircularLayer(64, this);
	m_pCircularLayer->CreateCircular(200);
	return CNode::Init();
}

void ClipRegion::CalculateRect()
{
	if(IsNeedUpdateRect())
	{
		CNode::CalculateRect();
		CreateRect8();
	}
	CNode::CalculateRect();
}

void ClipRegion::DrawNode(DrawKit* pKit)
{
	RECT r = GetRectI();
	HDC hMemDC = GetView()->GetMemDC();
	int i = (r.right - r.left) / 3;

	DrawShadow();
	SelectObject(hMemDC, IsLBDown() ? m_FrameDotPen2 : m_FrameDotPen1);
	MoveToEx(hMemDC, r.left + i, r.top + 1, NULL);
	LineTo(hMemDC, r.left + i, r.bottom);
	MoveToEx(hMemDC, r.right - i, r.top + 1, NULL);
	LineTo(hMemDC, r.right - i, r.bottom);
	i = (r.bottom - r.top) / 3;
	MoveToEx(hMemDC, r.left + 1, r.top + i, NULL);
	LineTo(hMemDC, r.right, r.top + i);
	MoveToEx(hMemDC, r.left + 1, r.bottom - i, NULL);
	LineTo(hMemDC, r.right, r.bottom - i);

	m_pCircularLayer->DrawImage(r.left, r.top, r.right-r.left, r.bottom-r.top);

	SelectObject(hMemDC, m_hFramePen);
	MoveToEx(hMemDC, r.left, r.top, NULL);
	LineTo(hMemDC, r.right, r.top);
	LineTo(hMemDC, r.right, r.bottom);
	LineTo(hMemDC, r.left, r.bottom);
	LineTo(hMemDC, r.left, r.top);

	for (auto iter = m_vecShowRect8.begin(); iter != m_vecShowRect8.end(); ++iter)
	{
		FillRect(hMemDC, &(*iter), m_FrameBrush);
	}
}

void ClipRegion::DrawShadow()
{
	CNode* parent = GetParent();
	assert(parent != NULL);
	RECT rf  =GetRect();
	RECT ri=dynamic_cast<CStaticImageNode* >(parent->GetChildByTag(CCutImageScene::TagMain))->GetImageLayer()->GetRect();

	POINT old;
	XFORM xfold;
	HDC hdc = GetView()->GetMemDC();
	GetWorldTransform(hdc, &xfold);
	ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
	SetViewportOrgEx(hdc, 0, 0, &old);

	// left
	if (rf.left > ri.left)
		m_pShadowLayer->DrawImage(ri.left, ri.top, rf.left - ri.left, ri.bottom - ri.top);
	// top
	if (rf.top > ri.top)
		m_pShadowLayer->DrawImage(rf.left, ri.top, ri.right - rf.left, rf.top - ri.top);

	// right
	if (rf.right < ri.right)
		m_pShadowLayer->DrawImage(rf.right, rf.top, ri.right - rf.right, ri.bottom - rf.top);

	// bottom
	if (rf.bottom < ri.bottom)
		m_pShadowLayer->DrawImage(rf.left, rf.bottom, rf.right - rf.left, ri.bottom - rf.bottom);
	SetViewportOrgEx(hdc, old.x, old.y, NULL);
	SetWorldTransform(hdc, &xfold);
}

void ClipRegion::CreateRect8()
{
	int which(0);
	while(which++<2)
	{
		RECT rect = (which==1?GetRectI():GetRect());
		auto& vec = (which==1?m_vecShowRect8:m_vecRect8);
		int x(rect.left),
			y(rect.top),
			w(rect.right - rect.left),
			h(rect.bottom - rect.top);

		/*
		0	7	6
		1		5
		2	3	4
		*/

		vec.clear();
		// left up
		rect.left = x - n;
		rect.right = rect.left + n;
		rect.top = y - n;
		rect.bottom = rect.top + n;
		vec.push_back(rect);
		// left middle
		rect.top = y + h / 2 - n / 2;
		rect.bottom = rect.top + n;
		vec.push_back(rect);
		// left down
		rect.top = y + h;
		rect.bottom = rect.top + n;
		vec.push_back(rect);;
		// down middle
		rect.left = x + w / 2 - n / 2;
		rect.right = rect.left + n;
		vec.push_back(rect);
		// right down
		rect.left = x + w;
		rect.right = rect.left + n;
		vec.push_back(rect);
		// right middle
		rect.top = y + h / 2 - n / 2;
		rect.bottom = rect.top + n;
		vec.push_back(rect);
		// right up
		rect.top = y - n;
		rect.bottom = rect.top + n;
		vec.push_back(rect);
		// up middle
		rect.left = x + w / 2 - n / 2;
		rect.right = rect.left + n;
		vec.push_back(rect);
	}
}

bool ClipRegion::IsPointINNode(POINT point)
{
	if (PtInRect(&GetRect(), point))
	{
		return true;
	}
	for (auto iter = m_vecRect8.begin(); iter != m_vecRect8.end(); ++iter)
	{
		if (PtInRect(&(*iter), point))
			return true;
	}
	return false;
}

void ClipRegion::MouseDown(POINT point, unsigned int flag, bool l)
{
	if (l)
	{
		SetCursor(m_hCursor);
		m_bStretchDown = false;
		m_iStretchIndex = -1;
		for (int i = 0; i < 8; ++i)
		if (PtInRect(&m_vecRect8[i], point))
		{
			m_bStretchDown = true;
			m_iStretchIndex = i;
			break;
		}
		m_pCircularLayer->CreateCircular(100);
		GetView()->Refresh();
	}
	CNode::MouseDown(point, flag, l);
}

void ClipRegion::MouseUp(POINT point, unsigned int flag, bool l)
{
	if(l)
	{
		m_bStretchDown=false;
		m_iStretchIndex = -1;
		m_pCircularLayer->CreateCircular(200);
		GetView()->Refresh();
	}
	CNode::MouseUp(point, flag, l);
}

void ClipRegion::MouseTravel(POINT point, unsigned int flag)
{
	m_hCursor = LoadCursor(NULL, IDC_SIZEALL);
	for (int i = 0; i < 8; ++i)
	{
		if (PtInRect(&m_vecRect8[i], point))
		{
			switch (i)
			{
			case 3:
			case 7:
				m_hCursor = LoadCursor(NULL, IDC_SIZENS);
				break;
			case 4:
			case 0:
				m_hCursor = LoadCursor(NULL, IDC_SIZENWSE);
				break;
			case 5:
			case 1:
				m_hCursor = LoadCursor(NULL, IDC_SIZEWE);
				break;
			case 6:
			case 2:
				m_hCursor = LoadCursor(NULL, IDC_SIZENESW);
				break;
			default:
				break;
			}
		}
	}

	//CNode::MouseTravel(point, flag);
	SetCursor(m_hCursor);
}

CCutImageScene::CCutImageScene():CShadowScene(3)
{

}

bool CCutImageScene::Init()
{
	RECT rect=GetRect();

	CColorLayer* pTitleContent=new CColorLayer(this);
	pTitleContent->CreateImageLayerByColor(0, 122, 204);
	pTitleContent->SetSize(rect.right - rect.left, 30);
	pTitleContent->SetPos((rect.right - rect.left) / 2.0f, 15.0f);
	pTitleContent->SetNCHitTest(HTCAPTION);

	rect.left+=10;
	rect.right-=120;
	rect.top+=30;
	{
		m_pMain=new CStaticImageNode(PresentCenter, this);
		CImageLayer* pLayer=new CImageLayer();
		if(pLayer->CreateImageLayerByFile(L"C:\\Users\\Think\\Desktop\\1.jpg"))
		{
			m_pMain->SetTag(TagMain);
			m_pMain->SetRect(rect);
			m_pMain->SetImageLayer(pLayer);
		}

		m_pHead=new ClipRegion(128, this);
		m_pHead->SetTag(TagHead);
		m_pHead->SetPos(rect.left+(rect.right-rect.left)/2.0f, rect.top+(rect.bottom-rect.top)/2.0f);
	}

	RECT r=rect;
	r.left = rect.right+10;
	r.right = r.left + 100;
	r.top += 30;
	r.bottom =r.top + 30;
	CTextLayer* pPreview=new CTextLayer(this);
	pPreview->SetText(L"预览");
	pPreview->SetRect(r);

	CImageLayer* pIL=new CImageLayer(this);
	pIL->SetTag(TagPreviewBig);
	r.top=r.bottom+10;
	r.bottom=r.top+100;
	pIL->SetRect(r);

	pIL =new CImageLayer(this);
	pIL->SetTag(TagPreviewSmall);
	r.top=r.bottom+10;
	r.bottom=r.top+60;
	pIL->SetSize(60, 60);
	pIL->SetPos(r.left+(r.right-r.left)/2, r.top+(r.bottom-r.top)/2);

	CButtonNode* pOK=new CButtonNode(this);
	pOK->SetTag(TagOK);
	pOK->SetText(L"确定");
	pOK->SetBorderColor(RGB(200, 200, 200), RGB(210, 210, 210));
	pOK->SetBgColor(RGB(240, 240, 240), RGB(56, 89, 245));
	pOK->SetCallback(std::bind(&CCutImageScene::OnButton, this, std::placeholders::_1));
	r.top = r.bottom + 10;
	r.bottom = r.top + 30;
	pOK->SetRect(r);

	CButtonNode* pCancel=new CButtonNode(this);
	pCancel->SetTag(TagCancel);
	pCancel->SetText(L"取消");
	pCancel->SetBorderColor(RGB(200, 200, 200), RGB(210, 210, 210));
	pCancel->SetBgColor(RGB(240, 240, 240), RGB(56, 89, 245));
	pCancel->SetCallback(std::bind(&CCutImageScene::OnButton, this, std::placeholders::_1));
	r.top = r.bottom + 10;
	r.bottom = r.top + 30;
	pCancel->SetRect(r);

	bool bRes=CScene::Init();
	UpdatePreviewImageNode();
	return bRes;
}

void CCutImageScene::MouseTravel(POINT point, unsigned int flag)
{
	CNode* pCurrentNode = GetCurrentNode();
	if(IsLBDown() && pCurrentNode && pCurrentNode->GetTag()==TagHead)
	{
		int dx = point.x - GetLastPoint().x;
		int dy = point.y - GetLastPoint().y;
		int  n;
		RECT rect_head = m_pHead->GetRect();
		RECT rect_show = m_pMain->GetImageLayer()->GetRect();

		if (m_pHead->IsStretchDown())
		{
			switch (m_pHead->GetStretchIndex())
			{
				// 查看 ClipRegion::CreateRect8，可以找到这些数字的意义
			case 0:
				if (dy == 0)
					return;
				rect_head.left += dy;
				rect_head.top += dy;
				break;
			case 1:
			case 2:
				if (dx == 0)
					return;
				rect_head.left += dx;
				rect_head.bottom -= dx;
				break;
			case 3:
				if (dy == 0)
					return;
				rect_head.right += dy;
				rect_head.bottom += dy;
				break;
			case 4:
			case 5:
				if (dx == 0)
					return;
				rect_head.right += dx;
				rect_head.bottom += dx;
				break;
			case 6:
			case 7:
				if (dy == 0)
					return;
				rect_head.top += dy;
				rect_head.right -= dy;
				break;
			}

			if (rect_head.right > rect_show.right)
			{
				n = rect_head.right - rect_show.right;
				rect_head.right -= n;
				rect_head.bottom -= n;
			}
			if (rect_head.bottom > rect_show.bottom)
			{
				n = rect_head.bottom - rect_show.bottom;
				rect_head.bottom -= n;
				rect_head.right -= n;
			}

			if (rect_head.top < rect_show.top)
			{
				n = rect_show.top - rect_head.top;
				rect_head.top += n;
				rect_head.right -= n;
			}
			if (rect_head.left < rect_show.left)
			{
				n = rect_show.left - rect_head.left;
				rect_head.left += n;
				rect_head.bottom -= n;
			}
		}
		else
		{
			rect_head.left += dx;
			rect_head.right += dx;
			rect_head.top += dy;
			rect_head.bottom += dy;

			n = 0;
			if (rect_head.right > rect_show.right)
				n = rect_head.right - rect_show.right;
			if (rect_head.left < rect_show.left)
				n = rect_head.left - rect_show.left;
			rect_head.left -= n;
			rect_head.right -= n;
			n = 0;
			if (rect_head.bottom > rect_show.bottom)
				n = rect_head.bottom - rect_show.bottom;
			if (rect_head.top < rect_show.top)
				n = rect_head.top - rect_show.top;
			rect_head.top -= n;
			rect_head.bottom -= n;
		}
		m_pHead->SetRect(rect_head);
		SetLastPoint(point);
		RefreshNode();
		return;
	}
	CScene::MouseTravel(point, flag);
}

void CCutImageScene::MouseUp(POINT point, unsigned int flag, bool l)
{
	CNode* pCurrentNode = GetCurrentNode();
	if(l && pCurrentNode && pCurrentNode->GetTag() == TagHead)
	{
		UpdatePreviewImageNode();
		RefreshNode();
	}
	CScene::MouseUp(point, flag, l);
}

void CCutImageScene::OnButton(CNode* pNode)
{
	if (pNode->GetTag() == TagOK)
	{
		CNode* p = FindFirstNode();
		while (1)
		{
			if (!p)
				break;
			TRACE1(L"node name is %s\n", p->GetNodeClassName().c_str());
			p = FindNextNode();
		}
	}
	PostMessage(GetView()->GetWnd(), WM_CLOSE, 0, 0);
}

std::unique_ptr<unsigned char[]> CCutImageScene::GetCutData(int& cw, int& ch, int& cbpp)
{
	CImageLayer* pI=m_pMain->GetImageLayer();
	auto& info=pI->GetImageInfo();
	RECT rect_head = m_pHead->GetRect();
	RECT rect_show = pI->GetRect();

	int x = rect_head.left - rect_show.left;
	int y = rect_head.top - rect_show.top;
	int w = rect_head.right - rect_head.left;
	int h = rect_head.bottom - rect_head.top;

	x = static_cast<int>(x * m_pMain->GetImageScaleW());
	y = static_cast<int>(y * m_pMain->GetImageScaleH());
	w = static_cast<int>(w * m_pMain->GetImageScaleW());
	h = static_cast<int>(h * m_pMain->GetImageScaleH());

	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (y + h > info.biHeight) 
		h = info.biHeight - y;
	if (x + w > info.biWidth) 
		w = info.biWidth - x;

	auto pData = pI->ImageData();
	auto bpp = info.biBitCount / 8;
	int stride = w*bpp;
	if (stride % 4 != 0)
	{
		stride = stride / 4 * 4 + 4;
	}
	int stride_org = info.biWidth*bpp;
	if (stride_org % 4 != 0)
	{
		stride_org = stride_org / 4 * 4 + 4;
	}
	std::unique_ptr<unsigned char[]> pTmp(new unsigned char[h*stride]);
	unsigned char *p1, *p2, *p3;
	for (int j = 0; j < h; ++j)
	{
		p1 = pData + (y + j)*stride_org;
		for (int i = 0; i < w; ++i)
		{
			p2 = p1 + (x + i)*bpp;
			p3 = pTmp.get() + j*stride + i*bpp;
			for (int k = 0; k < bpp; ++k)
			{
				*(p3 + k) = *(p2 + k);
			}
		}
	}

	cw = w;
	ch = h;
	cbpp = info.biBitCount;
	return pTmp;
}

void CCutImageScene::UpdatePreviewImageNode()
{
	int w, h, bpp;
	auto pData = GetCutData(w, h, bpp);
	if (!pData)
		return;

	auto pPreview = (CImageLayer*)GetChildByTag(TagPreviewBig);
	pPreview->CreateImageLayerByData(pData.get(), w, h, bpp, false, true);
	pPreview = (CImageLayer*)GetChildByTag(TagPreviewSmall);
	pPreview->CreateImageLayerByData(pData.get(), w, h, bpp, false, true);
}

LRESULT CCutImageWindow::CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed)
{
	if(message == WM_CREATE)
	{
		InitCutImage();
		//InitWeather();
	}

	if(m_pDir.get())
	{
		auto res=m_pDir->MessageProc(message, wParam, lParam, bProcessed);
		if(bProcessed)
		{
			return res;
		}
	}
	return CBaseWindow::CustomProc(hWnd, message, wParam, lParam, bProcessed);
}

//WS_TABSTOP
void CCutImageWindow::InitCutImage()
{
	LONG_PTR style=GetWindowLong(GetHWND(), GWL_STYLE);
	style &= ~WS_CAPTION;    // 去除非客户区域
	style &= ~WS_SIZEBOX;    // 禁止更改窗口大小, same as WS_THICKFRAME
	style &= ~WS_MAXIMIZEBOX;
	//SetWindowLongPtr(GetHWND(), GWL_STYLE, style);

	ReSize(600+6, 450+6, true);
	//ReSize(50, 50, true);

	//RECT r;
	//HRGN hRgn;
	//GetClientRect(GetHWND(), &r);
	//GetWindowRect(GetHWND(), &r);
	//hRgn=CreateRoundRectRgn(0, 0, r.right-r.left, r.bottom-r.top, 5, 5); // 窗口圆角
	//SetWindowRgn(GetHWND(), hRgn, false);
	//DeleteObject(hRgn);

	//CGDIView* pView=new CGDIViewAlpha();
	//pView->Init(GetHWND());
	//m_pDir.reset(new CDirector(pView));
	//m_pDir->RunScene(new CCutImageScene());

	//CGDIView* pView=new CGDIView();  
	//pView->Init(GetHWND());
	//m_pDir.reset(new CDirector(pView));
	//m_pDir->RunScene(new CTestScene());

	Node2DView* pView = new Node2DView();
	pView->Init(GetHWND());
	m_pDir.reset(new CDirector(pView));
	m_pDir->RunScene(new CTestScene2D());
}

void CCutImageWindow::InitWeather()
{
	ReSize(238, 248, true);
	LONG_PTR style=GetWindowLong(GetHWND(), GWL_STYLE);
	style &= ~WS_CAPTION;    // 去除非客户区域
	style &= ~WS_THICKFRAME; // 禁止更改窗口大小
	style &= ~WS_MAXIMIZEBOX;
	SetWindowLongPtr(m_hWnd, GWL_STYLE, style);

	//style = GetWindowLong(m_hWnd, GWL_EXSTYLE);
	//style |= WS_EX_TOOLWINDOW;
	//style &= ~WS_EX_APPWINDOW;
	//style= SetWindowLong(m_hWnd, GWL_EXSTYLE, style);

	RECT r;
	GetWindowRect(GetHWND(), &r);
	HRGN hRgn = CreateRoundRectRgn(0, 0, r.right-r.left, r.bottom-r.top, 2, 2);
	SetWindowRgn(GetHWND(), hRgn, true);
	DeleteObject(hRgn);

	CGDIView* pView=new CGDIView();
	pView->Init(GetHWND());
	m_pDir.reset(new CDirector(pView));
	m_pDir->RunScene(new WeatherScene());
}

bool CTestScene::Init()
{
	EnableCustomNCHitTest(false);

	CHLayout* pMain = new CHLayout(this);
	{
		//CButtonNode* p1 = new CButtonNode();
		//p1->SetText(L"sd");
		CVLayout* p1 = new CVLayout();
		p1->SetSizePolicy(SizePolicyExpanding);
		p1->SetSpacing(5);
		pMain->AddChild(p1);

		CButtonNode* p11 = new CButtonNode();
		p11->SetText(L"11");
		p11->SetSizePolicy(SizePolicyExpanding);
		p1->AddChild(p11);

		CButtonNode* p12 = new CButtonNode();
		p12->SetText(L"12");
		p12->SetSizePolicy(SizePolicyExpanding);
		p1->AddChild(p12);
	}

	CButtonNode* p2 = new CButtonNode();
	p2->SetText(L"sc", true);
	pMain->AddChild(p2);
	pMain->SetContentMargin(2, 20, 5, 30);
	pMain->SetSpacing(10);

	CButtonNode* p3=new CButtonNode();
	p3->SetText(L"确定", true);
	p3->SetPos(200, 200);
	pMain->AddChild(p3);


	return CScene::Init();
}

void CTestScene::DrawNode(DrawKit* pKit)
{
	{
		CScene::DrawNode(pKit);
		return;
	}
	RECT r=GetRect();
	HDC hMemDC=GetView()->GetMemDC();
	GetClientRect(GetView()->GetWnd(), &r);
	r.right -= 0;

	FillRect(hMemDC, &r, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

	r.right -= 1;
	FillRect(hMemDC, &r, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

	return;
	
	int n=10;

	if(n==0)
	{
		//gdi 划线
		//dib alpha
		HPEN hPen=CreatePen(PS_SOLID, 5, RGB(255, 0, 0));
		SelectObject(hMemDC, hPen);
		MoveToEx(hMemDC, 0, 0, NULL);
		LineTo(hMemDC, r.right, r.bottom);
	}
	else if(n==1)
	{
		// gdi 填充矩形
		HBRUSH hBrush=::CreateSolidBrush(RGB(0, 0, 255));
		FillRect(hMemDC, &r, hBrush);
	}
	else if(n==2)
	{
				// gdi 填充矩形
		HBRUSH hBrush=::CreateSolidBrush(RGB(0, 0, 255));
		FillRect(hMemDC, &r, hBrush);

		// gdi 字体渲染
		HFONT	hFont = CreateFont(20, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE,
		GB2312_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		FF_MODERN,
		L"微软雅黑");

		std::wstring s = L"预览WMHXYZCNTV我是谁";
		SelectObject(hMemDC, hFont);
		SetTextColor(hMemDC, RGB(0, 255, 0));
		DrawText(hMemDC, s.c_str(), s.length(), &r, DT_WORDBREAK);
	}


}

bool CTestScene2D::Init()
{
	EnableCustomNCHitTest(false);

	CNode2DTextLayer* pText = new CNode2DTextLayer(this);
	pText->CreateTextFormat(L"微软雅黑", 18);
	pText->SetText(L"确定");
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
