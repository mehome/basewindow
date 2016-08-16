#include "Thread.h"
#include "Log.h"

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
#pragma warning(pop)
}

CApplication::CApplication()
{
	SetThreadName(GetCurrentThreadId(), "gui");
}

CApplication::~CApplication()
{
}

int CApplication::Run()
{
	MSG msg;
	BOOL bRet;

	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			TRACE1("GetMessage return -1:%u\n", GetLastError());
			break;
		}
		if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}