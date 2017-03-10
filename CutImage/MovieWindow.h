#pragma once
#include "Mp3Player.h"

class CMovieShow : public CScene
{
public:
	bool Init();
};

class CMovieWindow : public CBaseWindow, public CApplication
{
public:
	CMovieWindow();
	~CMovieWindow();
	LRESULT CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed);
	int Run();
	bool OpenFile(const std::string& fileName);
protected:
	inline void MainLoop();
	bool WriteAudioData();
protected:
	LARGE_INTEGER m_liFreq;
	LARGE_INTEGER m_liLast;
	LARGE_INTEGER m_liNow;
	LARGE_INTEGER m_liInterval;
	std::unique_ptr<CDirector> m_pDir;
	std::unique_ptr<RingBuffer> m_pSoundBuf;
	CSimpleDecoder m_decoder;
	CSound m_sound;
	int m_iCountForAudio;
};
