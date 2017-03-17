#pragma once
#include <Windows.h>

class ISyncVideo
{
public:
	enum {
		DontShowThisFrameNow,
		DoShowThisFrameNow,
		SkiThisFrame_ShowNext
	};
	virtual ~ISyncVideo() {}
	virtual int IsSwitchToNextFrame(void*) = 0;
};

class CSyncVideoByFrameRate : public ISyncVideo
{
public:
	CSyncVideoByFrameRate(LARGE_INTEGER frameInterval);
	int IsSwitchToNextFrame(void*);
protected:
	LARGE_INTEGER m_liMaxDisplay;
	LARGE_INTEGER m_liMinDisplay;
	LARGE_INTEGER m_liLast;
	LARGE_INTEGER m_liError;
	LARGE_INTEGER m_liFrameInterval;
};

class CSyncVideoByAudioTime : public ISyncVideo
{
public:
	CSyncVideoByAudioTime();
	int IsSwitchToNextFrame(void*);
protected:
	double m_dError;
};