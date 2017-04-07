#pragma once

#include <ObjBase.h>

class CCustomStream : public IStream
{
public:
	CCustomStream(char* pData, DWORD len);
	~CCustomStream(void);
	// IUnknown
	STDMETHODIMP QueryInterface(REFIID iid, void** ppObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	// ISequentialStream
	STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
	STDMETHODIMP Write(const void *pv, ULONG cb, ULONG *pcbWritten);
	// IStream
	STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
	STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
	STDMETHODIMP Commit(DWORD grfCommitFlags);
	STDMETHODIMP Revert(void);
	STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	STDMETHODIMP Stat(STATSTG *pStg, DWORD grfStatFlag);
	STDMETHODIMP Clone(IStream **ppstm);
protected:
	ULONG m_uRefCount;
	LARGE_INTEGER m_liLen;
	char* m_pData;
	DWORD m_dwOffset;
};
