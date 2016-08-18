#pragma once
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include "View.h"

class CDirector;
class CScene;
class CNode;

typedef std::pair<float, float> NodePair;
typedef std::pair<int, int> NodePairInt;
typedef std::function<bool(CNode*)> NodeFinder;

enum NodeSizePolicy
{
	SizePolicyFixed,
	SizePolicyPrefered,
	SizePolicyExpanding,
};

class CNode
{
private:
	CNode(const CNode& r);
	CNode& operator=(const CNode& r);
	CNode& operator=(CNode&& rr);
public:
	explicit CNode(CNode* pParent=NULL);
	CNode(CNode&& rr);
	virtual ~CNode();

	virtual bool IsInitialized()const;
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

	virtual void SetParent(CNode* p);
	virtual CNode* GetParent()const;
	virtual void ChangeParent(CNode* pNew);

	virtual void SetAnchor(float x, float y);
	virtual NodePair GetAnchor()const;

	virtual void SetSize(float w, float h);
	virtual void SetSize(int w, int h);
	virtual NodePair GetSize()const;
	virtual void SetMinSize(int x, int y);
	virtual void SetMaxSize(int x, int y);
	virtual NodePairInt GetMinSize()const;
	virtual NodePairInt GetMaxSize()const;
	virtual void SetSizePolicy(NodeSizePolicy policy);
	virtual NodeSizePolicy GetSizePolicy()const;

	virtual void SetPos(float px, float py);
	virtual void SetPos(int x, int y);
	virtual NodePair GetPos()const;
	virtual void NeedUpdate();
	virtual bool IsUpdateNeeded()const;

	virtual bool AddChild(CNode* pNode);
	virtual bool AddChild(CNode* pNode, int order);
	virtual bool RemoveChild(CNode* pNode, bool bDelete=true);
	virtual bool RemoveChildAll(bool bDelete=true);
	virtual void SortChild();
	virtual CNode* GetChildByTag(int tag);
	virtual CNode* GetChildByFinder(NodeFinder  func);

	virtual bool Init();
	virtual bool Destroy();
	virtual void DrawNode();
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
	virtual CNode* GetCurrentNode();
private:
	bool m_bInitialized;
	bool m_bNeedUpdate;
	bool m_bVisible;
	int m_iData;
	int m_iTag;
	int m_iZOrder;
	int m_iNCHitTest;
	CNode* m_pParent;
	NodePair m_pairAnchor;
	NodePair m_pairSize;
	NodePair m_pairPos;
	NodePairInt m_pairMinSize;
	NodePairInt m_pairMaxSize;
	NodeSizePolicy m_sizePolicy;

	POINT m_pointLast;
	bool m_bLBDown;
	bool m_bRBDown;
	bool m_bMouseIN;
	CNode* m_pCurrentNode;
	std::vector<CNode* > m_Children;
protected:
	RECT m_rect;
};

class CScene : public CNode
{
public:
	CScene();
	CScene* GetScene();
	RECT GetRect();
	virtual CGDIView* GetView();
	virtual void SetView(CGDIView*);
	virtual CDirector* GetDirector();
	virtual void SetDirector(CDirector*);
	virtual LRESULT MessageProc(UINT, WPARAM, LPARAM, bool& bProcessed);
	virtual void DrawScene();
protected:
	CGDIView*  m_pView;
	CDirector* m_pDir;
};

class CShadowScene : public CScene
{
public:
	explicit CShadowScene(int shadowSize=3);
	void SetView(CGDIView*);
	void DrawScene();
	void ClearScene();
	void SwapScene();
	virtual void DrawShadow();
protected:
	int m_iShadowSize;
};

class CDirector
{
public:
	explicit CDirector(CGDIView* view);
	virtual ~CDirector();
	virtual void RunScene(CScene* scene);
	virtual LRESULT MessageProc(UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed);
protected:
	CGDIView* m_pCurrentView;
	CScene*   m_pCurrentScene;
};

class CImageLayer : public CNode
{
public:
	explicit CImageLayer(CNode* pParent=NULL);
	~CImageLayer();
	bool CreateImageLayerByData(unsigned char* pData, int w, int h, int bitcount, bool bUseImageSizeAsNodeSize=true);
	bool CreateImageLayerByFile(const std::wstring& sFileName);
	virtual bool CreateImageLayerByColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a=255);
	virtual void DrawImage(int dest_leftup_x, int dest_leftup_y, int dest_w, int dest_h, unsigned char opacity=255);
	void DrawNode();
	const std::wstring& GetNodeClassName()const;
	unsigned char* ImageData()const
	{
		return m_pData;
	}
	const BITMAPINFOHEADER& GetImageInfo()const
	{
		return m_Info;
	}
protected:
	BITMAPINFOHEADER m_Info;
	unsigned char* m_pData;
	HDC m_hDc;
	HBITMAP m_hBitmap;
};

class CTextLayer : public CNode
{
public:
	explicit CTextLayer(CNode* pParent=NULL);
	~CTextLayer();
	void SetFont(const LOGFONT& f);
	void SetFont(HFONT h);
	HFONT GetFont();
	void SetTextColor(COLORREF color);
	COLORREF GetTextColor()const;
	void SetAlignment(DWORD align);
	DWORD GetAlignment()const;
	void SetText(const std::wstring& sText, bool bUseTextSizeAsNodeSize=false);
	const std::wstring& GetText()const;
	SIZE GetTextSize();
	void DrawNode();
protected:
	HFONT m_hFont;
	COLORREF m_dwColor;
	DWORD m_dwAlignment;
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
	CStaticImageNode(int showType, CNode* pParent=NULL);
	void SetImageLayer(CImageLayer* pLayer);
	CImageLayer* GetImageLayer();
	float GetImageScaleW(){return m_sw;}
	float GetImageScaleH(){return m_sh;}
protected:
	PresentType m_presentType;
	float m_sw;
	float m_sh;
};

typedef std::function<void(CNode*)> ButtonCallback;
class CButtonNode : public CTextLayer
{
public:
	explicit CButtonNode(CNode* pParent=NULL);
	void SetCallback(ButtonCallback ck);
	void SetBgColor(const Gdiplus::Color& normal, const Gdiplus::Color& highlight);
	void SetBorderColor(const Gdiplus::Color&normal, const Gdiplus::Color& highlight);
	void SetBorderWidth(int width);
	void DrawNode();
	void MouseEnter();
	void MouseLeave();
	void MouseUp(POINT point, unsigned int flag, bool l);
protected:
	ButtonCallback m_callback;
	Gdiplus::Color m_bgNormal;
	Gdiplus::Color m_bgHighLight;
	Gdiplus::Color m_borderNormal;
	Gdiplus::Color m_borderHighLight;
	int m_iBorderWidth;
};