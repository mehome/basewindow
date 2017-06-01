#pragma once
#include "Mp3Player.h"
#include "SyncVideo.h"

class CMovieShow : public CScene
{
public:
	bool Init();
	void DrawNode(DrawKit* pDrawKit);
	void UpdateImage(RingBuffer*p, int w, int h);
	//void CalculateRect();
	void SetFrameSize(std::pair<int, int> size)
	{
		m_pairFrameSize = size;
	}
protected:
	CImageLayer* m_pImage;
	std::pair<int, int> m_pairFrameSize;
};

class CMovieWindow : public CBaseWindow, public CApplication
{
public:
	CMovieWindow();
	~CMovieWindow();
	LRESULT CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed);
	int Run();
	bool OpenFile(const std::string& fileName);
	void Pause();
protected:
	inline void MainLoop();
	bool WriteAudioData();
protected:
	LARGE_INTEGER m_liFreq;
	LARGE_INTEGER m_liLast;
	LARGE_INTEGER m_liNow;
	LARGE_INTEGER m_liInterval;
	double m_dRefreshGap;
	std::unique_ptr<CDirector> m_pDir;
	std::unique_ptr<RingBuffer> m_pSoundBuf;
	std::unique_ptr<RingBuffer> m_pCurrImage;
	FrameInfo m_ImageInfo;
	CDecodeLoop m_decoder;
	CSound m_sound;
	CMovieShow* m_pShow;
	std::unique_ptr<ISyncVideo> m_pSync;
	std::pair<double, double> m_pairForSyncAV;
	int m_iPlayStatue;
};
