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

const std::wstring& AppPath()
{
	static std::wstring path;

	if (path.empty())
	{
		wchar_t buf[1024] = { 0 };
		GetModuleFileNameW(NULL, buf, 1024);
		path = buf;
		path = path.substr(0, path.find_last_of(L'\\')+1);
	}
	return path;
}

CApplication::CApplication():m_threadId(0)
{
	m_threadId = GetCurrentThreadId();
	SetThreadName(m_threadId, "gui");
}

CApplication::~CApplication()
{
}

int CApplication::Run()
{
	MSG msg;
	
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!msg.hwnd && msg.message>=WM_USER)
		{
			HandleQueueMessage(msg);
			continue;
		}

		if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	TRACE("application run end\n");
	return 0;
}

int CApplication::HandleQueueMessage(const MSG& msg)
{
	TRACE1("queue message time is %u\n", msg.time);
	TRACE1("queue message type is %u\n", msg.message);
	if (msg.message == WM_Custom_Task)
	{
		CTask* pTask = (CTask*)(msg.wParam);
		if (pTask)
		{
			pTask->Do();
			delete pTask;
		}
	}
	return 0;
}

bool CApplication::Init()
{
	TRACE("application init\n");
	return true;
}

void CApplication::Destroy()
{
	TRACE("application destroy\n");
}

bool CApplication::PostTask(CTask* pTask)
{
	if (!pTask)
		return false;
	while (!PostThreadMessage(ThreadId(), WM_Custom_Task, (WPARAM)pTask, 0))
	{
		if (ERROR_INVALID_THREAD_ID == GetLastError())
			Sleep(0);
		else
			return false;
	}
	return true;
}

void CMessageLoop::RunTaskOnce(CTask* p)
{
	CMessageLoop* pOnce = new CMessageLoop(true);
	pOnce->Init();
	pOnce->PostTask(p);
	pOnce->Destroy();
}

CMessageLoop::CMessageLoop(bool bAutoDelete) :
	m_bRunning(false),
	m_bAutoDelete(bAutoDelete),
	m_hThread(NULL)
{
}

CMessageLoop::~CMessageLoop()
{
	if(m_hThread)
	{
		m_bRunning=false;
		if (!m_bAutoDelete)
		{
			WaitForSingleObject(m_hThread, 1000);
			CloseHandle(m_hThread);
		}
		m_hThread=NULL;
	}
}

int CMessageLoop::Run()
{
	TRACE("worker thread start\n");
	SetThreadName(ThreadId(), "messageloop");

	BOOL bRet;
	MSG msg;
	int n(0);
	while(m_bRunning)
	{
		// filter queue message only
		bRet=GetMessage(&msg, (HWND)-1, 0, 0);
		if(bRet==0 || bRet==-1)
		{
			m_bRunning=false;
			break;
		}

		HandleQueueMessage(msg);
	}

	while (++n<50)
	{
		if (PeekMessage(&msg, (HWND)-1, 0, 0, PM_REMOVE))
			HandleQueueMessage(msg);
	}

	TRACE("worker thread stop\n");
	m_bRunning=false;

	if (m_bAutoDelete)
	{
		HANDLE hHandle = m_hThread;
		delete this;
		CloseHandle(hHandle);
	}
	return 0;
}

unsigned int __stdcall ThreadFunc(void* p)
{
	CMessageLoop* pThread=static_cast<CMessageLoop*>(p);
	if(!pThread)
		return 0;
	return pThread->Run();
}

bool CMessageLoop::Init()
{
	CLockGuard<CSimpleLock> guard(&m_lock);
	
	if(m_bRunning)
		return false;

	m_hThread=(HANDLE)_beginthreadex(NULL, 0, ThreadFunc, this, 0, &m_threadId);
	m_bRunning=true;
	return true;
}

void CMessageLoop::Destroy()
{
	CLockGuard<CSimpleLock> guard(&m_lock);
	if(m_bRunning)
	{
		PostThreadMessage(ThreadId(), WM_QUIT, 0, 0);
		m_bRunning = false;
	}
}