#pragma once

#include <string>
#include <windows.h>
#include <MsHTML.h>
#include <Mshtmhst.h>
#include <ExDisp.h>
#include <ExDispid.h>
#include <BaseWindow.h>

class CBaseWebBrowser : public IOleClientSite,
	public IOleInPlaceFrame,
	public IOleInPlaceSite
{
public:
	CBaseWebBrowser();
	~CBaseWebBrowser(void);

	HRESULT EmbedBroswerObject(HWND hwnd);
	HRESULT UnEmbedBroswerObject();
	HRESULT DisplayHtmlPage(const std::wstring& szURL);
	HRESULT DisplayHtmlString(const std::wstring& szMes);
	HRESULT ExecuteJS(wchar_t* szMethodName,DISPPARAMS* pParam,VARIANT* pRes);
	IOleObject* GetOleObject();
	virtual IDocHostUIHandler* GetCustomUIHandler();
	// IUnknown
	virtual STDMETHODIMP QueryInterface(REFIID iid,void** ppObject);
	virtual STDMETHODIMP_(ULONG) AddRef();
	virtual STDMETHODIMP_(ULONG) Release();

	// IOleClientSite
    virtual STDMETHODIMP SaveObject(void);
    virtual STDMETHODIMP GetMoniker(DWORD dwAssign,DWORD dwWhichMoniker,IMoniker **ppmk);
    virtual STDMETHODIMP GetContainer(IOleContainer **ppContainer);
    virtual STDMETHODIMP ShowObject(void);
    virtual STDMETHODIMP OnShowWindow(BOOL fShow);
    virtual STDMETHODIMP RequestNewObjectLayout(void);

	// IOleWindow
	virtual STDMETHODIMP GetWindow(HWND *phwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
	// IOleInPlaceUIWindow
	virtual STDMETHODIMP GetBorder(LPRECT lprectBorder);
    virtual STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS pborderwidths);
	virtual STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS pborderwidths);
	virtual STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject *pActiveObject,LPCOLESTR pszObjName);
	// IOleInPlaceFrame
	virtual STDMETHODIMP InsertMenus( HMENU hmenuShared,LPOLEMENUGROUPWIDTHS lpMenuWidths);
	virtual STDMETHODIMP SetMenu(HMENU hmenuShared, HOLEMENU holemenu,HWND hwndActiveObject);
	virtual STDMETHODIMP RemoveMenus(HMENU hmenuShared);
	virtual STDMETHODIMP SetStatusText(LPCOLESTR pszStatusText);
	virtual STDMETHODIMP EnableModeless(BOOL fEnable);
	virtual STDMETHODIMP TranslateAccelerator(LPMSG lpmsg,WORD wID);

	// IOleInPlaceSite
	virtual STDMETHODIMP CanInPlaceActivate( void);
    virtual STDMETHODIMP OnInPlaceActivate( void);
    virtual STDMETHODIMP OnUIActivate( void);
    virtual STDMETHODIMP GetWindowContext(IOleInPlaceFrame **ppFrame,
           IOleInPlaceUIWindow **ppDoc,
           LPRECT lprcPosRect,
           LPRECT lprcClipRect,
           LPOLEINPLACEFRAMEINFO lpFrameInfo);
    virtual STDMETHODIMP Scroll(SIZE scrollExtant);
    virtual STDMETHODIMP OnUIDeactivate(BOOL fUndoable);
    virtual STDMETHODIMP OnInPlaceDeactivate( void);
    virtual STDMETHODIMP DiscardUndoState( void);
    virtual STDMETHODIMP DeactivateAndUndo( void);
    virtual STDMETHODIMP OnPosRectChange(LPCRECT lprcPosRect);
protected:
	HWND m_hWnd;
	IOleObject* m_pOleObject;
	DWebBrowserEvents2* m_pWBEvent2;
	unsigned long m_uiCount;
};

class CWebEventSink : public DWebBrowserEvents2
{
public:
	CWebEventSink(void);
	virtual ~CWebEventSink(void);  
	HRESULT StartAdvise(IWebBrowser2* pWB);
	HRESULT StopAdvise();
	virtual STDMETHODIMP OnBeforeNavigate2(IDispatch*,VARIANT*,VARIANT*,VARIANT*,VARIANT*,VARIANT*,VARIANT_BOOL*);
	virtual STDMETHODIMP OnDocumentComplete(IDispatch *pDisp, VARIANT *URL);
public:
	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID riid,void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	// IDispatch methods
	STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
	STDMETHODIMP GetTypeInfo(UINT iTInfo,LCID lcid,ITypeInfo **ppTInfo);
	STDMETHODIMP GetIDsOfNames(REFIID,LPOLESTR *,UINT,LCID,DISPID *);
	STDMETHODIMP Invoke(DISPID,REFIID,LCID,WORD,DISPPARAMS *,VARIANT *,EXCEPINFO *,UINT *);
protected:
	IConnectionPoint* m_pConnPoint;
	DWORD m_dwRef;
	DWORD m_dwCookie;
};

class CCustomUIHandler: public IDocHostUIHandler
{
public:
	CCustomUIHandler();
	~CCustomUIHandler();
	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid,void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	// IDocHostUIHandler
	STDMETHODIMP ShowContextMenu(DWORD dwID, POINT *ppt,IUnknown *pcmdtReserved,IDispatch *pdispReserved);
	STDMETHODIMP GetHostInfo(DOCHOSTUIINFO *pInfo);
	STDMETHODIMP ShowUI(DWORD dwID,
		IOleInPlaceActiveObject *pActiveObject,
		IOleCommandTarget *pCommandTarget,
		IOleInPlaceFrame *pFrame,
		IOleInPlaceUIWindow *pDoc);
	STDMETHODIMP HideUI( void);
	STDMETHODIMP UpdateUI( void);
	STDMETHODIMP EnableModeless(BOOL fEnable);
	STDMETHODIMP OnDocWindowActivate(BOOL fActivate);
	STDMETHODIMP OnFrameWindowActivate(BOOL fActivate);
	STDMETHODIMP ResizeBorder(LPCRECT prcBorder,IOleInPlaceUIWindow *pUIWindow,BOOL fRameWindow);
	STDMETHODIMP TranslateAccelerator(LPMSG lpMsg,const GUID *pguidCmdGroup,DWORD nCmdID);
	STDMETHODIMP GetOptionKeyPath(LPOLESTR *pchKey,DWORD dw);
	STDMETHODIMP GetDropTarget(IDropTarget *pDropTarget,IDropTarget **ppDropTarget);
	STDMETHODIMP GetExternal(IDispatch **ppDispatch);
	STDMETHODIMP TranslateUrl(DWORD dwTranslate,OLECHAR *pchURLIn,OLECHAR **ppchURLOut);
	STDMETHODIMP FilterDataObject(IDataObject *pDO,IDataObject **ppDORet);
protected:
	IOleObject* m_pOleObject;
	DWORD m_dwRef;
};

class CExternalObject:public IDispatch
{
public:
	static const DISPID m_dispidTestMethod;
	CExternalObject();
	~CExternalObject();
	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID riid,void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	// IDispatch methods
	STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
	STDMETHODIMP GetTypeInfo(UINT iTInfo,LCID lcid,ITypeInfo **ppTInfo);
	STDMETHODIMP GetIDsOfNames(REFIID,LPOLESTR *,UINT,LCID,DISPID *);
	STDMETHODIMP Invoke(DISPID,REFIID,LCID,WORD,DISPPARAMS *,VARIANT *,EXCEPINFO *,UINT *);
protected:
	DWORD m_dwRef;
};

class CWebBrowserWindow : public CBaseWindow
{
public:
	CWebBrowserWindow(const std::wstring& szURL=L"about:blank");
	~CWebBrowserWindow();
	LRESULT CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed);
	void OpenURL(const std::wstring& url);
protected:
	CBaseWebBrowser* m_pWebBrowser;
	std::wstring m_szURL;
};
