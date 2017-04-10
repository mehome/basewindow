#include "BaseWebBroswer.h"
#pragma comment(lib, "version.lib")

class CWBStorage:public IStorage
{
public:
	STDMETHODIMP QueryInterface(REFIID iid,void** ppObject)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP_(ULONG) AddRef()
	{
		return 1;
	}
	STDMETHODIMP_(ULONG) Release()
	{
		return 1;
	}
	STDMETHODIMP CreateStream( const OLECHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2,IStream **ppstm)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP OpenStream(const OLECHAR *pwcsName,void *reserved1,DWORD grfMode,DWORD reserved2,IStream **ppstm)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP CreateStorage( const OLECHAR *pwcsName, DWORD grfMode,DWORD reserved1,DWORD reserved2,IStorage **ppstg)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP OpenStorage(const OLECHAR *pwcsName,IStorage *pstgPriority,DWORD grfMode,SNB snbExclude,DWORD reserved,IStorage **ppstg)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP CopyTo(DWORD ciidExclude,const IID *rgiidExclude,SNB snbExclude,IStorage *pstgDest)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP MoveElementTo( const OLECHAR *pwcsName,IStorage *pstgDest,const OLECHAR *pwcsNewName,DWORD grfFlags)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP Commit(DWORD grfCommitFlags)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP Revert( void)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP EnumElements( DWORD reserved1,void *reserved2,DWORD reserved3,IEnumSTATSTG **ppenum)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP DestroyElement(const OLECHAR *pwcsName)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP RenameElement(const OLECHAR *pwcsOldName,const OLECHAR *pwcsNewName)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP SetElementTimes( const OLECHAR *pwcsName,const FILETIME *pctime,const FILETIME *patime,const FILETIME *pmtime)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP SetClass(REFCLSID clsid)
	{
		return S_OK;
	}
	STDMETHODIMP SetStateBits( DWORD grfStateBits,DWORD grfMask)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP Stat( STATSTG *pstatstg,DWORD grfStatFlag)
	{
		return E_NOTIMPL;
	}
};

CBaseWebBrowser::CBaseWebBrowser():m_hWnd(NULL),
	m_pOleObject(NULL),
	m_pWBEvent2(NULL),
	m_uiCount(0)
{
}

CBaseWebBrowser::~CBaseWebBrowser(void)
{
}

HRESULT CBaseWebBrowser::EmbedBroswerObject(HWND hwnd)
{
	HRESULT res;
	RECT rect;
	CWBStorage storage;

	IWebBrowser2* pWB2;
	IDispatch* pDoc;
	VARIANT var;

	m_hWnd=hwnd;
	res=OleCreate(CLSID_WebBrowser,IID_IOleObject,OLERENDER_DRAW,0,
		static_cast<IOleClientSite*>(this),&storage,(void**)&m_pOleObject);
	if(S_OK == res)
	{
		GetClientRect(hwnd,&rect);
		m_pOleObject->SetHostNames(L"test webbrowser",0);
		if(S_OK == OleSetContainedObject(m_pOleObject,TRUE)&&
			S_OK == m_pOleObject->DoVerb(OLEIVERB_SHOW,NULL,this,-1,hwnd,&rect)&&
			S_OK == m_pOleObject->QueryInterface(IID_IWebBrowser2,(void**)&pWB2))
		{
			pWB2->put_Left(0);
			pWB2->put_Top(0);
			pWB2->put_Width(rect.right);
			pWB2->put_Height(rect.bottom);
			//pWB2->put_Silent(VARIANT_TRUE);

			//监听事件
			if(!m_pWBEvent2)
			{
				auto pEventSink=new CWebEventSink();
				pEventSink->AddRef();
				pEventSink->StartAdvise(pWB2);
				m_pWBEvent2=pEventSink;
			}

			// 更改风格
			var.vt=VT_BSTR;
			var.bstrVal=SysAllocString(L"about:blank");
			res=pWB2->Navigate2(&var,0,0,0,0);
			res=pWB2->get_Document(&pDoc);
			if(SUCCEEDED(res))
			{
				IHTMLDocument2* pHTMLDoc2;
				ICustomDoc* pCustomDoc;
				
				res=pDoc->QueryInterface(IID_IHTMLDocument2,(void**)&pHTMLDoc2);
				res=pHTMLDoc2->QueryInterface(IID_ICustomDoc,(void**)&pCustomDoc);
				res=pCustomDoc->SetUIHandler(GetCustomUIHandler());
				pWB2->Refresh();
				//pWB2->Navigate2(&var,0,0,0,0);
				pCustomDoc->Release();
				pHTMLDoc2->Release();
				pDoc->Release();
			}
			SysFreeString(var.bstrVal);
			pWB2->Release();
		}
	}
	return res;
}

HRESULT CBaseWebBrowser::UnEmbedBroswerObject()
{
	if(m_pOleObject)
	{
		HRESULT res;
		IWebBrowser2* pWB2;
		IDispatch* pDoc;
		IHTMLDocument2* pHTMLDoc2;
		ICustomDoc* pCustomDoc;

		res=m_pOleObject->QueryInterface(IID_IWebBrowser2,(void**)&pWB2);
		if(SUCCEEDED(res))
		{
			res=pWB2->get_Document(&pDoc);
			res=pDoc->QueryInterface(IID_IHTMLDocument2,(void**)&pHTMLDoc2);
			res=pHTMLDoc2->QueryInterface(IID_ICustomDoc,(void**)&pCustomDoc);
			pCustomDoc->SetUIHandler(NULL);
			pWB2->Quit();

			pCustomDoc->Release();
			pHTMLDoc2->Release();
			pDoc->Release();
			pWB2->Release();
		}
	}

	if(m_pWBEvent2)
	{
		dynamic_cast<CWebEventSink*>(m_pWBEvent2)->StopAdvise();
		m_pWBEvent2->Release();
	}

	if(m_pOleObject)
	{
		m_pOleObject->Close(OLECLOSE_NOSAVE);
		m_pOleObject->Release();
	}

	return S_OK;
}

HRESULT CBaseWebBrowser::DisplayHtmlPage(const std::wstring& szURL)
{
	HRESULT hr(S_OK);
	VARIANT varURL;
	IWebBrowser2* pWB2;

	if(m_pOleObject)
	{
		hr=m_pOleObject->QueryInterface(IID_IWebBrowser2,(void**)&pWB2);
		if(SUCCEEDED(hr))
		{
			VariantInit(&varURL);
			varURL.vt=VT_BSTR;
			varURL.bstrVal=SysAllocString(szURL.c_str());

			pWB2->Navigate2(&varURL,0,0,0,0);
			SysFreeString(varURL.bstrVal);
			pWB2->Release();
		}
	}
	return hr;
}

HRESULT CBaseWebBrowser::DisplayHtmlString(const std::wstring& szMes)
{
	HRESULT hr=S_FALSE;
	IWebBrowser2* pWB2(0);
	IDispatch* pDoc(0);
	IHTMLDocument2* pDoc2(0);
	SAFEARRAY* pArr(0);
	VARIANT* pVar(0); 
	static SAFEARRAYBOUND bound={1,0};

	if(m_pOleObject)
	{
		hr=m_pOleObject->QueryInterface(IID_IWebBrowser2,(void**)&pWB2);
		if(FAILED(hr))
		{
			goto End;
		}
		pArr= SafeArrayCreate(VT_VARIANT,1,&bound);
		if(!pArr || S_OK!=SafeArrayAccessData(pArr,(void**)&pVar))
		{
			goto End;
		}
		hr=pWB2->get_Document(&pDoc);
		if(FAILED(hr))
		{
			goto End;
		}
		hr=pDoc->QueryInterface(IID_IHTMLDocument2,(void**)&pDoc2);
		if(FAILED(hr))
		{
			goto End;
		}

		pVar->vt=VT_BSTR;
		pVar->bstrVal=SysAllocString(szMes.c_str());
		hr=pDoc2->write(pArr);
	}

End:
	if(pArr)
		SafeArrayDestroy(pArr);
	if(pDoc2)
		pDoc2->Release();
	if(pDoc)
		pDoc->Release();
	if(pWB2)
		pWB2->Release();
	return hr;
}

HRESULT CBaseWebBrowser::ExecuteJS(wchar_t* szMethodName,DISPPARAMS* pParam,VARIANT* pRes)
{
	HRESULT hr(S_FALSE);
	IWebBrowser2* pWB2(0);
	IDispatch* pDoc(0);
	IHTMLDocument2* pHTMLDoc2(0);
	IHTMLWindow2* pHTMLWin2(0);

	if(!m_pOleObject || !szMethodName || !pParam)
		return hr;
	hr=m_pOleObject->QueryInterface(IID_IWebBrowser2,(void**)&pWB2);
	if(SUCCEEDED(hr))
	{
		pWB2->get_Document(&pDoc);
		pDoc->QueryInterface(IID_IHTMLDocument2,(void**)&pHTMLDoc2);
		pHTMLDoc2->get_parentWindow(&pHTMLWin2);

		DISPID id;
		wchar_t*p[1];
		p[0]=szMethodName;
		hr=pHTMLWin2->GetIDsOfNames(IID_NULL,p,1,LOCALE_SYSTEM_DEFAULT,&id);
		if(SUCCEEDED(hr))
		{
			hr=pHTMLWin2->Invoke(id,IID_NULL,LOCALE_SYSTEM_DEFAULT,DISPATCH_METHOD,pParam,pRes,NULL,NULL);
		}
	}

	if(pHTMLWin2)
		pHTMLWin2->Release();
	if(pHTMLDoc2)
		pHTMLDoc2->Release();
	if(pDoc)
		pDoc->Release();
	if(pWB2)
		pWB2->Release();
	return hr;
}

IOleObject* CBaseWebBrowser::GetOleObject()
{
	return m_pOleObject;
}

IDocHostUIHandler* CBaseWebBrowser::GetCustomUIHandler()
{
	return new CCustomUIHandler();
}

// IUnknown
STDMETHODIMP CBaseWebBrowser::QueryInterface(REFIID iid,void** ppObject)
{
	if(iid == IID_IUnknown)
	{
		*ppObject = this;
	}
	else if(iid == IID_IOleClientSite)
	{
		*ppObject = static_cast<IOleClientSite*>(this);
	}
	else if(iid == IID_IOleInPlaceSite)
	{
		*ppObject = static_cast<IOleInPlaceSite*>(this);
	}
	else
	{
		*ppObject=NULL;
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CBaseWebBrowser::AddRef()
{
	InterlockedIncrement(&m_uiCount);
	TRACE1("%u    addref\n", m_uiCount);
	return m_uiCount;
}

STDMETHODIMP_(ULONG) CBaseWebBrowser::Release()
{
	InterlockedDecrement(&m_uiCount);
	if(m_uiCount == 0)
	{
		delete this;
	}
	TRACE1("%u    addref\n", m_uiCount);
	return m_uiCount;
}

// IOleClientSite
STDMETHODIMP CBaseWebBrowser::SaveObject(void)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::GetMoniker(DWORD dwAssign,DWORD dwWhichMoniker,IMoniker **ppmk)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::GetContainer(IOleContainer **ppContainer)
{
	*ppContainer=NULL;
	return E_NOINTERFACE;
}

STDMETHODIMP CBaseWebBrowser::ShowObject(void)
{
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::OnShowWindow(BOOL fShow)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::RequestNewObjectLayout(void)
{
	return E_NOTIMPL;
}

// IOleWindow
STDMETHODIMP CBaseWebBrowser::GetWindow(HWND *phwnd)
{
	*phwnd=m_hWnd;
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}

// IOleInPlaceSite
STDMETHODIMP CBaseWebBrowser::CanInPlaceActivate( void)
{
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::OnInPlaceActivate( void)
{
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::OnUIActivate( void)
{
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::GetWindowContext(IOleInPlaceFrame **ppFrame,
	IOleInPlaceUIWindow **ppDoc,
	LPRECT lprcPosRect,
	LPRECT lprcClipRect,
	LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
	*ppFrame=static_cast<IOleInPlaceFrame*>(this);
	AddRef();
	*ppDoc=NULL;

	lpFrameInfo->fMDIApp=FALSE;
	lpFrameInfo->hwndFrame=m_hWnd;
	lpFrameInfo->haccel=NULL;
	lpFrameInfo->cAccelEntries=NULL;

	GetClientRect(m_hWnd,lprcPosRect);
	GetClientRect(m_hWnd,lprcClipRect);
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::Scroll(SIZE scrollExtant)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::OnUIDeactivate(BOOL fUndoable)
{
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::OnInPlaceDeactivate( void)
{
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::DiscardUndoState( void)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::DeactivateAndUndo( void)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::OnPosRectChange(LPCRECT lprcPosRect)
{
	return S_OK;
}

// IOleInPlaceUIWindow
STDMETHODIMP CBaseWebBrowser::GetBorder(LPRECT lprectBorder)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::SetActiveObject(IOleInPlaceActiveObject *pActiveObject,LPCOLESTR pszObjName)
{
	return S_OK;
}

// IOleInPlaceFrame
STDMETHODIMP CBaseWebBrowser::InsertMenus( HMENU hmenuShared,LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::SetMenu(HMENU hmenuShared, HOLEMENU holemenu,HWND hwndActiveObject)
{
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::RemoveMenus(HMENU hmenuShared)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseWebBrowser::SetStatusText(LPCOLESTR pszStatusText)
{
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::EnableModeless(BOOL fEnable)
{
	return S_OK;
}

STDMETHODIMP CBaseWebBrowser::TranslateAccelerator(LPMSG lpmsg,WORD wID)
{
	return E_NOTIMPL;
}

CWebEventSink::CWebEventSink(void):m_dwRef(0),
	m_dwCookie(0),
	m_pConnPoint(0)
{
}

CWebEventSink::~CWebEventSink(void)
{
}

HRESULT CWebEventSink::StartAdvise(IWebBrowser2* pWB2)
{
	HRESULT hr=S_OK;
	IConnectionPointContainer* pConnContainer(0);

	if(m_pConnPoint)
		return hr;

	hr=pWB2->QueryInterface(IID_IConnectionPointContainer,(void**)&pConnContainer);
	if(SUCCEEDED(hr))
	{
		hr=pConnContainer->FindConnectionPoint(DIID_DWebBrowserEvents2,&m_pConnPoint);
		if(SUCCEEDED(hr))
		{
			hr=m_pConnPoint->Advise(static_cast<IUnknown*>(this),&m_dwCookie);
		}
		pConnContainer->Release();
	}

	return hr;
}

HRESULT CWebEventSink::StopAdvise()
{
	if(m_pConnPoint)
	{
		m_pConnPoint->Unadvise(m_dwCookie);
		m_pConnPoint->Release();
		m_pConnPoint=NULL;
	}
	
	return S_OK;
}

STDMETHODIMP CWebEventSink::OnBeforeNavigate2(IDispatch *pDisp,
	VARIANT *pvUrl,
	VARIANT *pvFlags,
	VARIANT *pvTargetFrameName,
	VARIANT *pvPostData,
	VARIANT *pvHeaders,
	VARIANT_BOOL *pvCancel) 
{
	return S_OK;
}


STDMETHODIMP CWebEventSink::OnDocumentComplete(IDispatch *pDisp, VARIANT *URL)
{
	if(wcscmp(URL->bstrVal,L"about:blank")==0)
		return S_OK;
	return S_OK;
}

STDMETHODIMP CWebEventSink::QueryInterface(REFIID riid,void **ppvObject)
{
	if(IsBadWritePtr(ppvObject,sizeof(void*))) return E_POINTER;

	*ppvObject=NULL;
	if(IsEqualIID(riid,DIID_DWebBrowserEvents2))
	{
		*ppvObject = static_cast<DWebBrowserEvents2*>(this);
	}
	else if(IsEqualIID(riid,IID_IUnknown))
	{
		*ppvObject = static_cast<IUnknown*>(this);
	}
	else if(IsEqualIID(riid,IID_IDispatch))
	{
		*ppvObject = static_cast<IDispatch*>(this);
	}
	else
	{
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CWebEventSink::AddRef()
{
   InterlockedIncrement(&m_dwRef);
   return m_dwRef;
}

STDMETHODIMP_(ULONG) CWebEventSink::Release()
{
   InterlockedDecrement(&m_dwRef);
   if (m_dwRef == 0)
      delete this;
   return m_dwRef;
}

STDMETHODIMP CWebEventSink::GetTypeInfoCount(UINT *pctinfo)
{
	UNREFERENCED_PARAMETER(pctinfo);
	*pctinfo=0;
	return S_OK;
}

STDMETHODIMP CWebEventSink::GetTypeInfo(UINT iTInfo,LCID lcid,ITypeInfo **ppTInfo)
{
	UNREFERENCED_PARAMETER(iTInfo);
	UNREFERENCED_PARAMETER(lcid);
	UNREFERENCED_PARAMETER(ppTInfo);

	*ppTInfo = NULL;
	return E_NOTIMPL;
}

STDMETHODIMP CWebEventSink::GetIDsOfNames(REFIID riid,LPOLESTR *rgszNames,UINT cNames,LCID lcid,DISPID *rgDispId)
{
	UNREFERENCED_PARAMETER(riid);
	UNREFERENCED_PARAMETER(rgszNames);
	UNREFERENCED_PARAMETER(cNames);
	UNREFERENCED_PARAMETER(lcid);
	UNREFERENCED_PARAMETER(rgDispId);

	return E_NOTIMPL;
}

// This is called by IE to notify us of events
// Full documentation about all the events supported by DWebBrowserEvents2 can be found at
//  http://msdn.microsoft.com/en-us/library/aa768283(VS.85).aspx
STDMETHODIMP CWebEventSink::Invoke(DISPID dispIdMember,
	REFIID riid,
	LCID lcid,
	WORD wFlags,
	DISPPARAMS *pDispParams,
	VARIANT *pVarResult,
	EXCEPINFO *pExcepInfo,
UINT *puArgErr)
{
	UNREFERENCED_PARAMETER(lcid);
	UNREFERENCED_PARAMETER(wFlags);
	UNREFERENCED_PARAMETER(pVarResult);
	UNREFERENCED_PARAMETER(pExcepInfo);
	UNREFERENCED_PARAMETER(puArgErr);

	if(!IsEqualIID(riid,IID_NULL)) return DISP_E_UNKNOWNINTERFACE; // riid should always be IID_NULL

	switch (dispIdMember)
	{
		case DISPID_BEFORENAVIGATE2: 
			OnBeforeNavigate2(
				(IDispatch*)pDispParams->rgvarg[6].byref,
				(VARIANT*)pDispParams->rgvarg[5].pvarVal,
				(VARIANT*)pDispParams->rgvarg[4].pvarVal,
				(VARIANT*)pDispParams->rgvarg[3].pvarVal,
				(VARIANT*)pDispParams->rgvarg[2].pvarVal,
				(VARIANT*)pDispParams->rgvarg[1].pvarVal,
				(VARIANT_BOOL*)pDispParams->rgvarg[0].pboolVal
			);
			break;
		case DISPID_DOCUMENTCOMPLETE:
			OnDocumentComplete((IDispatch*)pDispParams->rgvarg[1].byref,(VARIANT*)pDispParams->rgvarg[0].pvarVal);
			break;
	}

	return S_OK;
}

CCustomUIHandler::CCustomUIHandler():m_dwRef(0)
{
}

CCustomUIHandler::~CCustomUIHandler()
{
}

STDMETHODIMP CCustomUIHandler::QueryInterface(REFIID riid,void **ppvObject)
{
	if(IsBadWritePtr(ppvObject,sizeof(void*))) return E_POINTER;

	*ppvObject=NULL;
	if(IsEqualGUID(riid,IID_IDocHostUIHandler))
	{
		*ppvObject=static_cast<IDocHostUIHandler*>(this);
	}
	else if(IsEqualGUID(riid,IID_IUnknown))
	{
		*ppvObject=static_cast<IUnknown*>(this);
	}
	else
	{
		return E_NOINTERFACE;
	}

	return S_OK;
}

STDMETHODIMP_(ULONG) CCustomUIHandler::AddRef()
{
	++m_dwRef;
	return m_dwRef;
}

STDMETHODIMP_(ULONG) CCustomUIHandler::Release()
{
	--m_dwRef;
	if(m_dwRef == 0)
		delete this;
	return m_dwRef;
}

STDMETHODIMP CCustomUIHandler::ShowContextMenu(DWORD dwID, POINT *ppt,IUnknown *pcmdtReserved,IDispatch *pdispReserved)
{
	return S_OK;
}

STDMETHODIMP CCustomUIHandler::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
	pInfo->dwDoubleClick = sizeof(DOCHOSTUIINFO);
	pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER | /*DOCHOSTUIFLAG_SCROLL_NO*/DOCHOSTUIFLAG_FLAT_SCROLLBAR;
	pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

	return S_OK;
}

STDMETHODIMP CCustomUIHandler::ShowUI(DWORD dwID,
	IOleInPlaceActiveObject *pActiveObject,
	IOleCommandTarget *pCommandTarget,
	IOleInPlaceFrame *pFrame,
	IOleInPlaceUIWindow *pDoc)
{
	return S_OK;
}

STDMETHODIMP CCustomUIHandler::HideUI(void)
{
	return S_OK;
}

STDMETHODIMP CCustomUIHandler::UpdateUI(void)
{
	return S_OK;
}

STDMETHODIMP CCustomUIHandler::EnableModeless(BOOL fEnable)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomUIHandler::OnDocWindowActivate(BOOL fActivate)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomUIHandler::OnFrameWindowActivate(BOOL fActivate)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomUIHandler::ResizeBorder(LPCRECT prcBorder,IOleInPlaceUIWindow *pUIWindow,BOOL fRameWindow)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomUIHandler::TranslateAccelerator(LPMSG lpMsg,const GUID *pguidCmdGroup,DWORD nCmdID)
{
	return S_FALSE;
}

STDMETHODIMP CCustomUIHandler::GetOptionKeyPath(LPOLESTR *pchKey,DWORD dw)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomUIHandler::GetDropTarget(IDropTarget *pDropTarget,IDropTarget **ppDropTarget)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomUIHandler::GetExternal(IDispatch **ppDispatch)
{
	IDispatch* pDisp=new CExternalObject();
	pDisp->AddRef();
	*ppDispatch=pDisp;
	return S_OK;
}

STDMETHODIMP CCustomUIHandler::TranslateUrl(DWORD dwTranslate,OLECHAR *pchURLIn,OLECHAR **ppchURLOut)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomUIHandler::FilterDataObject(IDataObject *pDO,IDataObject **ppDORet)
{
	return E_NOTIMPL;
}

const DISPID CExternalObject::m_dispidTestMethod=12;

CExternalObject::CExternalObject():m_dwRef(0)
{
}

CExternalObject::~CExternalObject()
{
}

STDMETHODIMP CExternalObject::QueryInterface(REFIID riid,void **ppvObject)
{
	*ppvObject=NULL;

	if(IsEqualGUID(riid,IID_IUnknown))
		*ppvObject=static_cast<IUnknown*>(this);
	else if(IsEqualGUID(riid,IID_IDispatch))
		*ppvObject=static_cast<IDispatch*>(this);

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}
	else
		return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CExternalObject::AddRef()
{
	++m_dwRef;
	return m_dwRef;
}

STDMETHODIMP_(ULONG) CExternalObject::Release()
{
	--m_dwRef;
	if(m_dwRef < 1)
		delete this;
	return m_dwRef;
}

STDMETHODIMP CExternalObject::GetTypeInfoCount(UINT *pctinfo)
{
	*pctinfo=0;
	return S_OK;
}

STDMETHODIMP CExternalObject::GetTypeInfo(UINT iTInfo,LCID lcid,ITypeInfo **ppTInfo)
{
	*ppTInfo=NULL;
	return S_OK;
}

STDMETHODIMP CExternalObject::GetIDsOfNames(REFIID riid,LPOLESTR *rgszNames,UINT cNames,LCID lcid,DISPID *rgDispId)
{
	bool bOK=true;
	LPOLESTR pstr;
	for(UINT i=0; i<cNames; ++i)
	{
		pstr=rgszNames[i];
		if(wcscmp(pstr,L"TestMethod")==0)
			rgDispId[i]=m_dispidTestMethod;
		else
		{
			rgDispId[i]=-1;
			bOK=false;
		}
	}

	return bOK?S_OK:DISP_E_UNKNOWNNAME;
}

STDMETHODIMP CExternalObject::Invoke(DISPID dispIdMember,
	REFIID riid,
	LCID lcid,
	WORD wFlags,
	DISPPARAMS *pDispParams,
	VARIANT *pVarResult,
	EXCEPINFO *pExcepInfo,
	UINT *puArgErr)
{
	switch(dispIdMember)
	{
	case m_dispidTestMethod:
		pVarResult->vt=VT_I4;
		pVarResult->intVal=1001;
		break;
	}

	return S_OK;
}

CWebBrowserWindow::CWebBrowserWindow(const std::wstring& szURL):m_pWebBrowser(NULL),
	m_szURL(szURL)
{
}

CWebBrowserWindow::~CWebBrowserWindow()
{
}

LRESULT CWebBrowserWindow::CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed)
{
	switch(message)
	{
	case WM_SIZE:
		if(m_pWebBrowser)
		{
			HRESULT hr;
			RECT rect;
			IOleInPlaceObject* pInPlaceObj;
			IOleObject* pObj=m_pWebBrowser->GetOleObject();
			if(!pObj)
			{
				break;
			}
			hr=pObj->QueryInterface(IID_IOleInPlaceObject,(void**)&pInPlaceObj);
			if(S_OK == hr)
			{
				GetClientRect(hWnd,&rect);
				hr=pInPlaceObj->SetObjectRects(&rect,&rect);
				pInPlaceObj->Release();
			}
		}
		break;
	case WM_ERASEBKGND:
		bProcessed=true;
		return 1;
	case WM_TIMER:
		{
			DISPPARAMS para;
			VARIANT para_1;
			VARIANT res;

			para_1.vt=VT_BSTR;
			para_1.bstrVal=SysAllocString(L"<div><span>hello ,i am long. 我们，哈哈哈。 就让哈哈哈哈</span></div>");

			para.cArgs=1;
			para.rgvarg=&para_1;
			para.cNamedArgs=0;
			para.rgdispidNamedArgs=NULL;

			m_pWebBrowser->ExecuteJS(L"onAddMessage",&para,&res);
			SysFreeString(para_1.bstrVal);
		}
		KillTimer(hWnd, wParam);
		break;
	case WM_CREATE:
		m_pWebBrowser=new CBaseWebBrowser();
		m_pWebBrowser->AddRef();
		m_pWebBrowser->EmbedBroswerObject(hWnd);
		m_pWebBrowser->DisplayHtmlPage(m_szURL);
		//SetTimer(hWnd,1,300,NULL);
		break;
	case WM_DESTROY:
		m_pWebBrowser->UnEmbedBroswerObject();
		m_pWebBrowser->Release();
		break;
	}
	return CBaseWindow::CustomProc(hWnd, message, wParam, lParam, bProcessed);
}

void CWebBrowserWindow::OpenURL(const std::wstring& url)
{
	if(m_pWebBrowser)
	{
		m_pWebBrowser->DisplayHtmlPage(url);
	}
}

int CWebBrowserWindow::CheckRegister_FeatureBrowserEmulation()
{
	std::unique_ptr<wchar_t> buf(new wchar_t[2048]);
	GetModuleFileNameW(NULL, buf.get(), 2048);
	std::wstring appName = std::wstring(wcsrchr(buf.get(), L'\\') + 1); // get app name


	DWORD verHandle(0), verSize(0), version(0), versionEnum;
	verSize = GetFileVersionInfoSize(L"mshtml.dll", &verHandle);
	if (!verSize)
	{
		return 0;
	}
	if (verSize > 2048)
	{
		buf.reset(new wchar_t[verSize]);
	}
	if (GetFileVersionInfo(L"mshtml.dll", verHandle, verSize, buf.get()))
	{
		UINT   size = 0;
		LPBYTE lpBuffer = NULL;
		if (VerQueryValue(buf.get(), L"\\", (VOID FAR* FAR*)&lpBuffer, &size))
		{
			if (size)
			{
				VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
				if (verInfo->dwSignature == 0xfeef04bd)
				{
					version = (verInfo->dwFileVersionMS >> 16) & 0xffff; // get ie major version
					//(verInfo->dwFileVersionMS >> 16) & 0xffff,
					//(verInfo->dwFileVersionMS >> 0) & 0xffff,
					//(verInfo->dwFileVersionLS >> 16) & 0xffff,
					//(verInfo->dwFileVersionLS >> 0) & 0xffff
					if (version == 11)
						versionEnum = 11001;
					else if (version == 10)
						versionEnum = 10001;
					else if (version == 9)
						versionEnum = 9999;
					else if (version == 8)
						versionEnum = 8888;
					else 
						versionEnum = 7000;
				}
			}
		}
	}

	if (version == 0)
	{
		return 0;
	}
	HKEY hKey;
	LSTATUS res;
	res = RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", 0, KEY_QUERY_VALUE| KEY_SET_VALUE, &hKey);
	if (res != ERROR_SUCCESS)
	{
		return 0;
	}
	verSize = 2048;
	res = RegQueryValueEx(hKey, appName.c_str(), NULL, &verHandle/*get type*/, (LPBYTE)buf.get(), &verSize);
	if (res != ERROR_SUCCESS && res != 2)
	{
		RegCloseKey(hKey);
		return 0;
	}
	if (verHandle != REG_DWORD || *((DWORD*)buf.get()) < versionEnum)
	{
		res = RegSetValueEx(hKey, appName.c_str(), 0, REG_DWORD, (BYTE*)&versionEnum, 4);
	}
	RegCloseKey(hKey);

	return res == ERROR_SUCCESS;
}