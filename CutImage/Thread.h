#pragma once

#include <Windows.h>
#include <string>
#include "Task.h"

#define WM_Custom_Task WM_USER+1234

void SetThreadName(unsigned int threadId, const char* pName);
const std::wstring& AppPath();

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
	virtual bool PostTask(CTask* pTask);
protected:
	unsigned int m_threadId;
};

class CMessageLoop : public CApplication
{
public:
	static void RunTaskOnce(CTask* p);
	CMessageLoop(bool bAutoDelete=false);
	~CMessageLoop();
	int Run();
	bool Init();
	void Destroy();
	bool IsRunning()const { return m_bRunning; }
protected:
	bool m_bRunning;
	bool m_bAutoDelete;
	HANDLE m_hThread;
	CSimpleLock m_lock;
};