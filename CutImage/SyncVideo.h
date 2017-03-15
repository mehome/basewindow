#pragma once
#include <Windows.h>

class ISyncVideo
{
public:
	virtual ~ISyncVideo() = 0;
	virtual int IsSwitchToNextFrame(LARGE_INTEGER&) = 0;
};

class CSyncVideoByFrameRate : public ISyncVideo
{
public:
	~CSyncVideoByFrameRate();
};