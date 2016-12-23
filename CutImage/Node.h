#pragma once
#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "View.h"

class CDirector;
class CScene;
class CNode;

typedef std::pair<float, float> NodePair;
typedef std::function<bool(CNode*)> NodeFinder;
typedef std::vector<CNode* > NodeChild;
typedef Gdiplus::RectF NodeRectF;

enum NodeSizePolicy
{
	SizePolicyFixed,
	SizePolicyPrefered,
	SizePolicyExpanding,
};

enum NodeUpdateFlag
{
	UpdateFlagClean,
	UpdateFlagReSize,
	UpdateFlagReLocation = 1,
	UpdateFlagReSortchild,
	UpdateFlagReLayout
};

struct DrawKit
{
	CNode* pParent;
	CGDIView* pView;
	CDirector* pDirect;
};

class CNode
{
private:
	CNode(const CNode& r);
	CNode& operator=(const CNode& r);
	CNode& operator=(CNode&& rr);
public:
	explicit CNode(CNode* pParent = NULL);
	CNode(CNode&& rr);
	virtual ~CNode();

	virtual void SetVisible(bool b);
	virtual bool IsVisible()const;

	virtual void SetData(int data);
	virtual int  GetData()const;

	virtual void SetTag(int tag);
	virtual int  GetTag()const;

	virtual void SetZOrder(int order);
	virtual int GetZOrder()const;

	virtual void SetNCHitTest(int value);
	virtual int GetNCHitTest()const;

	virtual void SetRect(const RECT& rect);
	virtual RECT GetRect();
	virtual const NodeRectF& GetRectF();
	virtual const RECT& GetRectI();

	virtual void SetParent(CNode* p);
	virtual CNode* GetParent()const;
	virtual void ChangeParent(CNode* pNew);

	virtual void SetAnchor(float x, float y);
	virtual NodePair GetAnchor()const;

	virtual void SetSize(float w, float h);
	virtual void SetSize(int w, int h);
	virtual const NodePair& GetSize();
	virtual void SetMinSize(float x, float y);
	virtual void SetMaxSize(float x, float y);
	virtual const NodePair& GetMinSize()const;
	virtual const NodePair& GetMaxSize()const;
	virtual void SetSizePolicy(NodeSizePolicy policy);
	virtual NodeSizePolicy GetSizePolicy()const;

	virtual void SetPos(float px, float py);
	virtual void SetPos(int x, int y);
	virtual const NodePair& GetPos();

	virtual void SetScale(float sx, float sy);
	virtual const NodePair& GetScale()const;
	virtual void SetRotate(float r);
	virtual float GetRotate()const;
	        const NodePair& GetRorotateTri();

	virtual void NeedUpdate(NodeUpdateFlag flag);
	virtual bool IsNeedUpdateRect()const;

	virtual bool AddChild(CNode* pNode);
	virtual bool AddChild(CNode* pNode, int order);
	virtual bool RemoveChild(CNode* pNode, bool bDelete = true);
	virtual bool RemoveChildAll(bool bDelete = true);
	virtual void SortChild();
	virtual CNode* GetChildByTag(int tag);
	virtual CNode* GetChildByFinder(NodeFinder  func);
	virtual CNode* FindFirstNode();
	virtual CNode* FindNextNode();

	virtual bool Init();
	virtual bool Destroy();
	virtual void DrawNode(DrawKit* pDrawKit);
	virtual void RefreshNode();
	virtual CScene* GetScene();
	virtual CGDIView* GetView();
	virtual CDirector* GetDirector();
	virtual const std::wstring& GetNodeClassName()const;

	virtual void MouseEnter();
	virtual void MouseLeave();
	virtual void MouseTravel(POINT point, unsigned int flag);
	virtual void MouseDown(POINT point, unsigned int flag, bool l);
	virtual void MouseUp(POINT point, unsigned int flag, bool l);
	virtual bool IsLBDown()const;
	virtual bool IsRBDown()const;
	virtual bool IsMouseIN()const;
	virtual bool IsPointINNode(POINT point);
	virtual POINT GetLastPoint()const;
	virtual void SetLastPoint(POINT p);
	virtual CNode* GetCurrentNode();
protected:
	const NodeChild& Child()const { return m_Children; }
	virtual void CalculateRect();
private:
	bool m_bNeedInit;
	bool m_bNeedUpdateRect;
	bool m_bNeedSortChild;
	bool m_bVisible;
	int m_iData;
	int m_iTag;
	int m_iZOrder;
	int m_iNCHitTest;
	CNode* m_pParent;
	NodePair m_pairAnchor;
	NodePair m_pairSize;
	NodePair m_pairPos;
	NodePair m_pairScale;
	float m_floatRotate;
	NodePair m_pairRoatateTri;
	RECT m_rect;
	RECT m_rectI;
	NodeRectF m_rectF;
	NodePair m_pairMinSize;
	NodePair m_pairMaxSize;
	NodeSizePolicy m_sizePolicy;

	POINT m_pointLast;
	bool m_bLBDown;
	bool m_bRBDown;
	bool m_bMouseIN;
	CNode* m_pCurrentNode;
	NodeChild m_Children;
	int m_iFindIndex;
};

class CHLayout :public CNode
{
public:
	explicit CHLayout(CNode* parent = NULL);
	bool AddChild(CNode* pNode);
	bool RemoveChild(CNode* pNode, bool bDelete);
	void NeedUpdate(NodeUpdateFlag flag);
	RECT GetRect();
	const NodePair& GetPos();
	const NodePair& GetSize();
	void DrawNode(DrawKit* pKit);
	void SetContentMargin(int l, int t, int r, int b);
	void SetSpacing(int spacing);
protected:
	virtual void ReLayout();
	int m_iMarginLeft;
	int m_iMarginTop;
	int m_iMarginRight;
	int m_iMarginBottom;
	int m_iSpacing;
private:
	bool m_bNeedReLayout;
	bool m_bUsedAsNode;
};

class CVLayout: public CHLayout
{
public:
	explicit CVLayout(CNode* parent = NULL):CHLayout(parent) {}
protected:
	void ReLayout();
};

class CScene : public CNode
{
public:
	CScene();
	CScene* GetScene();
	virtual CGDIView* GetView();
	virtual void SetView(CGDIView*);
	virtual CDirector* GetDirector();
	virtual void SetDirector(CDirector*);
	virtual LRESULT MessageProc(UINT, WPARAM, LPARAM, bool& bProcessed);
	virtual void DrawScene();
	virtual bool EnableCustomNCHitTest(bool value);
protected:
	DrawKit m_DrawKit;
	bool m_bCustomNCHitTest;
};

class CShadowScene : public CScene
{
public:
	explicit CShadowScene(int shadowSize = 3);
	void SetView(CGDIView*);
	void DrawScene();
	void ClearScene();
	void SwapScene();
	virtual void DrawShadow();
private:
	int m_iShadowSize;
};

class CDirector
{
public:
	explicit CDirector(CGDIView* view);
	virtual ~CDirector();
	virtual void RunScene(CScene* scene);
	virtual LRESULT MessageProc(UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed);
private:
	CGDIView* m_pCurrentView;
	CScene*   m_pCurrentScene;
};

class CColorLayer : public CNode
{
public:
	explicit CColorLayer(CNode* parent = NULL);
	virtual bool CreateImageLayerByColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a=255);
	void DrawNode(DrawKit* pKit);
protected:
	std::unique_ptr<void, win_handle_deleter<0>> m_hBrush;
};

class CImageLayer : public CColorLayer
{
public:
	explicit CImageLayer(CNode* pParent = NULL);
	~CImageLayer();
	bool CreateImageLayerByBitmap(Gdiplus::Bitmap* pBitmap);
	bool CreateImageLayerByStream(IStream* pStream);
	bool CreateImageLayerByFile(const std::wstring& sFileName);
	bool CreateImageLayerByData(unsigned char* pData, int w, int h, int bitcount, bool bUseImageSizeAsNodeSize = true);
	bool CreateImageLayerByColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);
	bool ScaleImageInside(int new_w, int new_h);
	void DrawImage(int dest_leftup_x, int dest_leftup_y, int dest_w, int dest_h, unsigned char opacity = 255);
	void DrawNode(DrawKit* pKit);
	const std::wstring& GetNodeClassName()const;
	unsigned char* ImageData()const { return m_pData; }
	const BITMAPINFOHEADER& GetImageInfo()const { return m_Info; }
protected:
	BITMAPINFOHEADER m_Info;
	unsigned char* m_pData;
	HDC m_hDc;
	HBITMAP m_hBitmap;
};

class CTextLayer : public CNode
{
public:
	explicit CTextLayer(CNode* pParent = NULL);
	~CTextLayer();
	void SetFont(const LOGFONT& f);
	void SetFont(HFONT h);
	HFONT GetFont();
	void SetTextColor(COLORREF color);
	COLORREF GetTextColor()const;
	void SetText(const std::wstring& sText, bool bUseTextSizeAsNodeSize = false);
	const std::wstring& GetText()const;
	SIZE GetTextSize();
	void DrawNode(DrawKit* pKit);
protected:
	HFONT m_hFont;
	COLORREF m_color;
	std::wstring m_szText;
};

enum PresentType
{
	PresentCenter,  // 在节点上居中显示
	PresentFillNode // 填充整个节点
};

class CStaticImageNode : public CNode
{
public:
	CStaticImageNode(int showType, CNode* pParent = NULL);
	void SetSize(float w, float h);
	void SetImageLayer(CImageLayer* pLayer);
	CImageLayer* GetImageLayer();
	float GetImageScaleW() { return m_sw; }
	float GetImageScaleH() { return m_sh; }
protected:
	void PutImage();
protected:
	PresentType m_presentType;
	float m_sw;
	float m_sh;
};

typedef std::function<void(CNode*)> ButtonCallback;
class CButtonNode : public CTextLayer
{
public:
	explicit CButtonNode(CNode* pParent = NULL);
	void SetCallback(ButtonCallback ck);
	void SetBgColor(COLORREF normal, COLORREF highlight);
	void SetBorderColor(COLORREF normal, COLORREF highlight);
	void SetBorderWidth(int width);
	void DrawNode(DrawKit* pKit);
	void MouseEnter();
	void MouseLeave();
	void MouseUp(POINT point, unsigned int flag, bool l);
protected:
	ButtonCallback m_callback;
	std::unique_ptr<void, win_handle_deleter<0>> m_bgNormal;
	std::unique_ptr<void, win_handle_deleter<0>> m_bgHighLight;
	std::unique_ptr<void, win_handle_deleter<0>> m_borderNormal;
	std::unique_ptr<void, win_handle_deleter<0>> m_borderHighLight;
	int m_iBorderWidth;
};