#include "CustomStream.h"


CCustomStream::CCustomStream(char* pData, DWORD len)
	: m_uRefCount(1),
	m_pData(pData),
	m_dwOffset(0)
{
	m_liLen.HighPart = 0;
	m_liLen.LowPart = len;
}

CCustomStream::~CCustomStream(void)
{
	delete[] m_pData;
}

STDMETHODIMP CCustomStream::QueryInterface(REFIID iid,void** ppObject)
{
	*ppObject = NULL;
	if(IsEqualGUID(IID_IUnknown, iid))
	{
		*ppObject = this;
	}
	else if(IsEqualGUID(IID_IStream, iid))
	{
		*ppObject = static_cast<IStream*>(this);
	}
	else if(IsEqualGUID(IID_ISequentialStream, iid))
	{
		*ppObject = static_cast<ISequentialStream*>(this);
	}

	if(!*ppObject)
		return E_NOINTERFACE;
	AddRef();
	return  S_OK;
}

STDMETHODIMP_(ULONG) CCustomStream::AddRef()
{
	return InterlockedIncrement(&m_uRefCount);
}

STDMETHODIMP_(ULONG) CCustomStream::Release()
{
	auto res = InterlockedDecrement(&m_uRefCount);
	if(res < 1)
		delete this;
	return res;
}

STDMETHODIMP CCustomStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	DWORD dwReaded;

	if (m_dwOffset + cb < m_liLen.LowPart)
		dwReaded = cb;
	else
		dwReaded = m_liLen.LowPart - m_dwOffset;
	memcpy(pv, m_pData + m_dwOffset, dwReaded);
	m_dwOffset += dwReaded;
	if (pcbRead)
		*pcbRead = dwReaded;
	return S_OK;
}

STDMETHODIMP CCustomStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	if (dwOrigin == FILE_BEGIN)
	{
		m_dwOffset = dlibMove.LowPart;
		if (m_dwOffset >= m_liLen.LowPart)
			return S_FALSE;
	}
	else if (dwOrigin == FILE_CURRENT || dwOrigin == FILE_END)
	{
		int offset = (int)m_dwOffset;
		offset += (int)dlibMove.LowPart;
		if (offset < 0 && offset >= (int)m_liLen.LowPart)
			return S_FALSE;
		m_dwOffset = (DWORD)offset;
	}

	if (plibNewPosition)
	{
		plibNewPosition->HighPart = 0;
		plibNewPosition->LowPart = m_dwOffset;
	}

	return S_OK;
}

STDMETHODIMP CCustomStream::SetSize(ULARGE_INTEGER libNewSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomStream::Commit(DWORD grfCommitFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomStream::Revert( void)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCustomStream::Stat(STATSTG *pStg, DWORD grfStatFlag)
{
	pStg->type = STGTY_LOCKBYTES;
	pStg->cbSize.QuadPart = (ULONGLONG)m_liLen.QuadPart;
	pStg->grfLocksSupported = LOCK_WRITE ;

	return S_OK;
}

STDMETHODIMP CCustomStream::Clone(IStream **ppstm)
{
	return E_NOTIMPL;
}