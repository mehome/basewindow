#pragma once
class CLog
{
public:
	static void LogDebug(const char* pFormat, ...);
	static void LogDebug(const wchar_t* pFormat, ...);
};

#ifdef _DEBUG
#define TRACE(p) CLog::LogDebug(p);
#define TRACE1(p, v1) CLog::LogDebug(p, v1);
#define TRACE2(p, v1, v2) CLog::LogDebug(p, v1, v2);
#define TRACE3(p, v1, v2, v3) CLog::LogDebug(p, v1, v2, v3);
#else
#define TRACE(p)
#define TRACE1(p, v1)
#define TRACE2(p, v1, v2)
#define TRACE3(p, v1, v2, v3)
#endif

