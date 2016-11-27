#pragma once
#include "Node.h"
#include "BaseWindow.h"

class CircularLayer : public CImageLayer
{
public:
	CircularLayer(int r, CNode* pParent);
	void CreateCircular(unsigned char opcticy);
};

class ClipRegion :public CNode
{
public:
	ClipRegion(int size, CNode* pParent, int line_width = 1, COLORREF color = RGB(34, 56, 255));
	~ClipRegion();

	bool Init();
	void DrawNode(DrawKit* pKit);
	RECT GetRect();
	bool IsPointINNode(POINT point);
	void MouseTravel(POINT point, unsigned int flag);
	void MouseDown(POINT point, unsigned int flag, bool l);
	void MouseUp(POINT point, unsigned int flag, bool l);
	bool IsStretchDown()const { return m_bStretchDown; }
	int GetStretchIndex()const { return m_iStretchIndex; }
protected:
	void CreateRect8(); // 8 指8个用于拉伸的正方形
	void DrawShadow();
protected:
	HPEN m_hFramePen;
	HPEN m_FrameDotPen1;
	HPEN m_FrameDotPen2;
	HBRUSH m_FrameBrush;
	bool m_bStretchDown;
	int m_iStretchIndex;
	HCURSOR m_hCursor;
	const int n;
	std::vector<RECT> m_vecRect8;
	CImageLayer* m_pShadowLayer;
	CircularLayer* m_pCircularLayer;
};

class CCutImageScene : public CShadowScene
{
public:
	enum
	{
		TagPreviewBig=1,
		TagPreviewSmall,
		TagOK,
		TagCancel,
		TagMain,
		TagHead
	};
	CCutImageScene();
	bool Init();
	void MouseTravel(POINT point, unsigned int flag);
	void MouseUp(POINT point, unsigned int flag, bool l);
protected:
	void OnButton(CNode* pNode);
	std::unique_ptr<unsigned char[]> GetCutData(int& w, int& h, int& bpp);
	void UpdatePreviewImageNode();
protected:
	CStaticImageNode* m_pMain;
	ClipRegion* m_pHead;
};

class CCutImageWindow : public CBaseWindow
{
public:
	void InitCutImage();
	void InitWeather();
	LRESULT CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed);
protected:
	std::unique_ptr<CDirector> m_pDir;
};

class CTestScene : public CScene
{
public:
	bool Init();
	void DrawNode(DrawKit* pKit);
};