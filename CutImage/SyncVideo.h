#pragma once
#include <Windows.h>

class ISyncVideo
{
public:
	virtual ~ISyncVideo() {}
	virtual int IsSwitchToNextFrame(LARGE_INTEGER&) = 0;
};

class CSyncVideoByFrameRate : public ISyncVideo
{
public:
	CSyncVideoByFrameRate(LARGE_INTEGER frameInterval, LARGE_INTEGER displayInterval);
	int IsSwitchToNextFrame(LARGE_INTEGER&);
protected:
	LARGE_INTEGER m_liMaxDisplay;
	LARGE_INTEGER m_liMinDisplay;
	LARGE_INTEGER m_liLast;
	LARGE_INTEGER m_liError;
	LARGE_INTEGER m_liFrameInterval;
	LARGE_INTEGER m_liDisplayInterval;
};