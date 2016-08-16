#include "Thread.h"
#include "Log.h"
#include <process.h>

void SetThreadName(unsigned int threadId, const char* pName)
{
	if (!pName)
		return;

	const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	} THREADNAME_INFO;
#pragma pack(pop)

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = pName;
	info.dwThreadID = threadId;
	info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{
	}
#pragma warning(default: 6320 6322)
#pragma warning(pop)
}

CThread::CThread():m_bRunning(false),
	m_hThread(NULL),
	m_threadId(0)
{
}

CThread::~CThread()
{
	if(m_hThread)
	{
		WaitForSingleObject(m_hThread, 1000);
		CloseHandle(m_hThread);
		m_hThread=NULL;
	}
}

int CThread::Run()
{
	TRACE("worker thread start\n");

	BOOL bRet;
	MSG msg;
	while(1)
	{
		bRet=GetMessage(&msg, (HWND)-1, 0, 0);
		if(bRet==0 || bRet==-1)
		{
			m_bRunning=false;
			break;
		}
	}

	TRACE("worker thread stop\n");
	m_bRunning=false;
	return 0;
}

unsigned int __stdcall ThreadFunc(void* p)
{
	CThread* pThread=static_cast<CThread*>(p);
	if(!pThread)
		return 0;
	return pThread->Run();
}

bool CThread::Init()
{
	CLockGuard<CSimpleLock> guard(&m_lock);
	
	if(m_bRunning || m_hThread != NULL)
		return false;

	if(m_hThread)
	{
		CloseHandle(m_hThread);
		m_hThread=NULL;
	}
	m_hThread=(HANDLE)_beginthreadex(NULL, 0, ThreadFunc, this, 0, &m_threadId);
	m_bRunning=true;
	return true;
}

void CThread::Destroy()
{
	CLockGuard<CSimpleLock> guard(&m_lock);
	if(m_bRunning)
		PostThreadMessage(ThreadId(), WM_QUIT, 0, 0);
}