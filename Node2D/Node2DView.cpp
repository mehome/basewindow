#include "Node2DView.h"


Node2DView::Node2DView():
	m_pD2DFactory(NULL)
{
}


Node2DView::~Node2DView()
{
}

bool Node2DView::Init(HWND hWnd)
{
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	return  CGDIView::Init(hWnd);
}