#pragma once
#include "Node/View.h"
#include <d2d1.h>
#include <dwrite.h>
#include "common.h"

class Node2DDeclear NodeDWrite
{
public:
	static NodeDWrite& Instance();
	NodeDWrite();
	~NodeDWrite();
	IDWriteFactory* GetDWrireFactory() { return m_pDWriteFactory; }
protected:
	IDWriteFactory *m_pDWriteFactory;
};

class Node2DDeclear Node2DView : public CGDIView
{
public:
	Node2DView();
	~Node2DView();
public:
	bool Init(HWND hWnd);
	void Clear();
	void WndRectChanged();

	void ClearBuffer();
	void SwapBuffer();

	NodeDWrite* GetNodeDWrite() { return m_pDWrite; }
	ID2D1RenderTarget* GetRenderTarget() { return m_pRenderTarget; }
protected:
	ID2D1Factory *m_pD2DFactory;
	ID2D1HwndRenderTarget *m_pRenderTarget;
	NodeDWrite* m_pDWrite;
};

