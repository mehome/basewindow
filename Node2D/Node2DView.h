#pragma once
#include <View.h>
#include <d2d1.h>
#include "common.h"

class Node2DDeclear Node2DView : protected CGDIView
{
public:
	Node2DView();
	~Node2DView();
public:
	bool Init(HWND hWnd);
protected:
	ID2D1Factory *m_pD2DFactory;
};

