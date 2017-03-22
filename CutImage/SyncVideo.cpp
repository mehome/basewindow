#include "SyncVideo.h"
#include <map>

CSyncVideoByFrameRate::CSyncVideoByFrameRate(LARGE_INTEGER frameInterval)
{
	m_liError.QuadPart = 0;
	m_liLast.QuadPart = 0;
	m_liFrameInterval.QuadPart = frameInterval.QuadPart;

	LARGE_INTEGER fre;
	QueryPerformanceFrequency(&fre);

	m_liMaxDisplay.QuadPart = (LONGLONG)(0.5*fre.QuadPart);
	m_liMinDisplay.QuadPart = (LONGLONG)(0.01*fre.QuadPart);
}

int CSyncVideoByFrameRate::IsSwitchToNextFrame(void* now)
{
	if (m_bPausePlay)
		return DontShowThisFrameNow;

	LARGE_INTEGER d;
	int res(DontShowThisFrameNow);

	d.QuadPart = ((LARGE_INTEGER*)now)->QuadPart - m_liLast.QuadPart;
	if (d.QuadPart < m_liMinDisplay.QuadPart)
	{
		return 0;
	}

	if (d.QuadPart > m_liMaxDisplay.QuadPart)
	{
		m_liError.QuadPart = 0;
		res = DoShowThisFrameNow;
		goto End;
	}

	d.QuadPart -= m_liFrameInterval.QuadPart;
	if (d.QuadPart >= 0)
	{
		m_liError.QuadPart += d.QuadPart;
		res = DoShowThisFrameNow;
	}
	else if (d.QuadPart + m_liError.QuadPart > 0)
	{
		m_liError.QuadPart += d.QuadPart;
		res = DoShowThisFrameNow;
	}
End:
	if (res == DoShowThisFrameNow)
	{
		m_liLast = *(LARGE_INTEGER*)now;
	}
	return res;
}

CSyncVideoByAudioTime::CSyncVideoByAudioTime(double fr, double gap):
	m_dFrameRate(fr),
	m_dDisplayGap(gap)
{
}

int CSyncVideoByAudioTime::IsSwitchToNextFrame(void* now)
{
	if (m_bPausePlay)
		return DontShowThisFrameNow;

	double d;
	std::pair<double, double>* pInfo = static_cast<std::pair<double, double>*>(now);

	// v time - a time
	d = pInfo->second - pInfo->first;

	if (d > 0)
	{
		if (d < abs(pInfo->first + m_dDisplayGap - pInfo->second))
			return DoShowThisFrameNow;
		else
			return DontShowThisFrameNow;
	}
	else
	{
		if (d < -0.5)
		{
			return SkiThisFrame_ShowNext;
		}
		return DoShowThisFrameNow;
	}
}