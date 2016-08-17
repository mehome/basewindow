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

class CTask
{
public:
	virtual void Do() = 0;
};

template<typename T>
class CTask0
{
public:
	typedef void (T::*F)(void);
public:
	CTask0(T* pObj, F func):m_pObj(pObj),m_func(func)
	{
	}
	void Do()
	{
		(m_pObj->*m_func)();
	}
protected:
	T* m_pObj;
	F m_func;
};

class CApplication
{
public:
	CApplication();
	virtual ~CApplication();
	virtual int Run(int n=0);
	virtual int HandleQueueMessage(const MSG& msg);
	virtual bool Init();
	virtual void Destroy();
	unsigned int ThreadId()const { return m_threadId; }
protected:
	unsigned int m_threadId;
};

class CThread : public CApplication
{
public:
	CThread();
	~CThread();
	int Run();
	bool Init();
	void Destroy();
	bool IsRunning()const { return m_bRunning; }
protected:
	bool m_bRunning;
	HANDLE m_hThread;
	CSimpleLock m_lock;
};