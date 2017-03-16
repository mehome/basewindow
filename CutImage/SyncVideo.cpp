#include "SyncVideo.h"

CSyncVideoByFrameRate::CSyncVideoByFrameRate(LARGE_INTEGER frameInterval, LARGE_INTEGER displayInterval)
{
	m_liError.QuadPart = 0;
	m_liLast.QuadPart = 0;
	m_liDisplayInterval.QuadPart = displayInterval.QuadPart;
	m_liFrameInterval.QuadPart = frameInterval.QuadPart;

	LARGE_INTEGER fre;
	QueryPerformanceFrequency(&fre);

	m_liMaxDisplay.QuadPart = 0.5*fre.QuadPart;
	m_liMinDisplay.QuadPart = 0.01*fre.QuadPart;
}

int CSyncVideoByFrameRate::IsSwitchToNextFrame(LARGE_INTEGER& now)
{
	LARGE_INTEGER d;
	int res(0);

	d.QuadPart = now.QuadPart - m_liLast.QuadPart;
	if (d.QuadPart < m_liMinDisplay.QuadPart)
	{
		return 0;
	}

	if (d.QuadPart > m_liMaxDisplay.QuadPart)
	{
		m_liError.QuadPart = 0;
		res = 1;
		goto End;
	}

	d.QuadPart -= m_liFrameInterval.QuadPart;
	if (d.QuadPart >= 0)
	{
		m_liError.QuadPart += d.QuadPart;
		res = 1;
	}
	else if (d.QuadPart + m_liError.QuadPart > 0)
	{
		m_liError.QuadPart += d.QuadPart;
		res = 1;
	}
End:
	if (res == 1)
	{
		m_liLast = now;
	}
	return res;
}