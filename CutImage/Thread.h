#pragma once

#include <Windows.h>

void SetThreadName(unsigned int threadId, const char* pName);

class CSimpleLock
{
public:
	CSimpleLock() { InitializeCriticalSection(&m_cs); }
	~CSimpleLock() { DeleteCriticalSection(&m_cs); }
	void Lock() { EnterCriticalSection(&m_cs); }
	void UnLock() { LeaveCriticalSection(&m_cs); }
protected:
	CRITICAL_SECTION m_cs;
};

template<typename T>
class CLockGuard
{
public:
	explicit CLockGuard(T* pObj):m_pObj(pObj)
	{
		if(m_pObj) m_pObj->Lock();
	}
	~CLockGuard()
	{
		if(m_pObj) m_pObj->UnLock();
	}
private:
	CLockGuard(const CLockGuard& r);
protected:
	T* m_pObj;
};

class CApplication
{
public:
	CApplication();
	virtual ~CApplication();
	virtual int Run();
	virtual int HandleQueueMessage(const MSG& msg);
	virtual bool Init();
	virtual void Destroy();
	unsigned int ThreadId()const { return m_threadId; }
protected:
	unsigned int m_threadId;
};

class CMessageLoop : public CApplication
{
public:
	CMessageLoop();
	~CMessageLoop();
	int Run();
	bool Init();
	void Destroy();
	bool IsRunning()const { return m_bRunning; }
protected:
	bool m_bRunning;
	HANDLE m_hThread;
	CSimpleLock m_lock;
};