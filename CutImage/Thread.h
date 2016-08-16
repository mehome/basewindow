#pragma once

#include <Windows.h>
#include <map>

void SetThreadName(unsigned int threadId, const char* pName);

class CSimpleLock
{
public:
	CSimpleLock() { InitializeCriticalSection(&m_cs); }
	~CSimpleLock() { DeleteCriticalSection(&m_cs); }
	void Enter() { EnterCriticalSection(&m_cs); }
	void Leave() { LeaveCriticalSection(&m_cs); }
protected:
	CRITICAL_SECTION m_cs;
};

class CApplication
{
public:
	CApplication();
	virtual ~CApplication();
	virtual int Run();
};
