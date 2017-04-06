#include "Log.h"
#include <cstdio>
#include <cstdarg>
#include <Windows.h>

void CLog::LogDebug(const char* pFormat, ...)
{
	const int buflen(1024);
	char buf[buflen]={0};
	va_list ap;

	if(!pFormat)
		return;

	va_start(ap, pFormat);
	vsprintf_s(buf, buflen, pFormat, ap);
	va_end(ap);

	OutputDebugStringA(buf);
}

void CLog::LogDebug(const wchar_t* pFormat, ...)
{
	const int buflen(1024);
	wchar_t buf[buflen]={0};
	va_list ap;

	if(!pFormat)
		return;

	va_start(ap, pFormat);
	vswprintf_s(buf,buflen,pFormat,ap);
	va_end(ap);

	OutputDebugStringW(buf);
}
