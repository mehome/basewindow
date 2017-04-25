#include "MovieWindow.h"
#include <windowsx.h>

bool CMovieShow::Init()
{
	EnableCustomNCHitTest(false);

	m_pImage = new CImageLayer();
	m_pImage->SetSizePolicy(SizePolicyExpanding);
	m_pImage->CreateImageLayerByColor(0, 0, 0);
	m_pImage->SetVisible(false);
	AddChild(m_pImage);
	return CScene::Init();
}

void CMovieShow::DrawNode(DrawKit* pDrawKit)
{
	//CScene::DrawNode(pDrawKit);
	m_pImage->DrawImage(0, 0, m_pImage->GetImageInfo().biWidth, m_pImage->GetImageInfo().biHeight);
}

void CMovieShow::UpdateImage(RingBuffer*p, int w, int h)
{
	if (m_pImage->GetImageInfo().biWidth != w || m_pImage->GetImageInfo().biHeight != h)
	{
		m_pImage->CreateImageLayerByData((unsigned char*)p->Data(), w, h, 24);
	}
	else
	{
		memcpy(m_pImage->ImageData(), p->Data(), p->ReadableBufferLen());
	}
	p->Reset();
}

CMovieWindow::CMovieWindow()
{
	QueryPerformanceFrequency(&m_liFreq);
	m_dRefreshGap = 1.0 / 50.0;
	m_iPlayStatue = 0;
}

CMovieWindow::~CMovieWindow()
{
	if (m_decoder.IsRunning())
	{
		m_decoder.Destroy();
	}
}

LRESULT CMovieWindow::CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed)
{
	if (message == WM_TIMER)
	{
		bProcessed = true;
		if (!WriteAudioData() && m_sound.UnPlayedDataLen() < m_sound.SoundFormat().nAvgBytesPerSec*0.4)
		{
			m_sound.Stop();
			KillTimer(hWnd, (UINT_PTR)this);
		}
		return 0;
	}
	else if (message == WM_SIZING || message == WM_MOVING)
	{
		MainLoop();
	}
	else if (message == WM_LBUTTONDOWN)
	{
		Pause();
	}
	else if (message == WM_RBUTTONDOWN)
	{
		bool bPause(false);
		if (m_iPlayStatue == 2)
		{
			// 处于暂停中
			Pause();
			bPause = true;
		}

		int x = GET_X_LPARAM(lParam);
		if (m_decoder.HasAudio())
		{
			KillTimer(GetHWND(), (UINT_PTR)this);
			m_sound.Stop();
			if (m_decoder.SeekTime(m_decoder.GetDurationAll()*x / m_ImageInfo.width, m_sound.PlayedTime()))
			{
				m_sound.Seek();
				m_sound.SetAudioBaseTime(m_decoder.AudioBaseTime());
				m_pCurrImage->Reset();
				m_pSoundBuf->Reset();
				m_sound.Start();
				if (WriteAudioData())
				{
					SetTimer(GetHWND(), (UINT_PTR)this, 400, NULL);
					m_liLast.QuadPart = 0;
					MainLoop();
				}
			}
		}
		else
		{
			if (m_decoder.SeekTime(1.0*m_decoder.GetDurationAll()*x / m_ImageInfo.width, m_ImageInfo.pts))
			{
				m_pCurrImage->Reset();
				m_liLast.QuadPart = 0;
				MainLoop();
			}
		}

		if (bPause)
		{
			// 继续暂停
			Pause();
		}
	}
	else if (message == WM_CREATE)
	{
		RECT r1, r2;
		GetWindowRect(hWnd, &r1);
		GetClientRect(hWnd, &r2);
		ReSize(1280 + (r1.right - r1.left) - (r2.right - r2.left),
			720 + (r1.bottom - r1.top) - (r2.bottom - r2.top),
			true);
		SetWindowText(GetHWND(), TEXT("Movie"));

		CGDIView* pView = new CGDIView();
		pView->Init(GetHWND());
		m_pDir.reset(new CDirector(pView));
		m_pShow = new CMovieShow();
		m_pDir->RunScene(m_pShow);

		OpenFile("e:\\1.mkv");
	}
	else if (message == WM_CLOSE)
	{
		m_sound.Stop();
	}

	if (m_pDir.get())
	{
		auto res = m_pDir->MessageProc(message, wParam, lParam, bProcessed);
		if (bProcessed)
		{
			return res;
		}
	}
	return CBaseWindow::CustomProc(hWnd, message, wParam, lParam, bProcessed);
}

int CMovieWindow::Run()
{
	QueryPerformanceCounter(&m_liLast);
	m_liInterval.QuadPart = LONGLONG(m_dRefreshGap * m_liFreq.QuadPart);

	MSG msg;
	while (true)
	{
		if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			MainLoop();
			continue;
		}

		if (msg.message == WM_QUIT)
			break;
		if (msg.hwnd == NULL && msg.message > WM_USER)
			HandleQueueMessage(msg);
		if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

bool CMovieWindow::OpenFile(const std::string& fileName)
{
	m_decoder.Destroy();
	m_decoder.Clean();
	m_sound.Stop();
	m_sound.Clear();
	KillTimer(GetHWND(), (UINT_PTR)this);
	m_iPlayStatue = 0;

	auto pIo = new CFileMappingIO(fileName);
	pIo->CreateBuffer(40960);
	m_decoder.SetCustomIOContext(pIo);
	if (m_decoder.LoadFile(fileName))
	{
		AudioParams outAudioParam;
		outAudioParam.sample_rate = m_decoder.GetSampleRate();
		m_decoder.ConfigureAudioOut(&outAudioParam);
		m_decoder.ConfigureVideoOut();

		if (m_decoder.HasVideo())
		{
			m_decoder.DecodeVideo(NULL, m_ImageInfo);
			m_pCurrImage.reset(new RingBuffer(m_ImageInfo.dataSize));
			m_decoder.DecodeVideo(m_pCurrImage.get(), m_ImageInfo);
			m_pShow->UpdateImage(m_pCurrImage.get(), m_ImageInfo.width, m_ImageInfo.height);
		}

		if (m_decoder.HasAudio())
		{
			PCMWAVEFORMAT info;
			info.wBitsPerSample = 16;
			info.wf.nBlockAlign = 4;
			info.wf.nChannels = 2;
			info.wf.nSamplesPerSec = outAudioParam.sample_rate;
			info.wf.wFormatTag = WAVE_FORMAT_PCM;
			info.wf.nAvgBytesPerSec = outAudioParam.sample_rate * 4;
			m_pSoundBuf.reset(new RingBuffer(info.wf.nAvgBytesPerSec));
			if (!m_sound.Initialize(info.wf, info.wBitsPerSample, info.wf.nAvgBytesPerSec * 3, GetHWND()))
			{
				return false;
			}
			m_sound.Start();

			int n;
			DWORD t1, t2;
			m_decoder.DecodeAudio(m_pSoundBuf.get(), n);
			if (n > 0 && m_sound.Write(m_pSoundBuf.get(), t1, t2) != 0)
			{
				SetTimer(GetHWND(), (UINT_PTR)this, 400, NULL);
			}

			if (m_decoder.HasVideo())
			{
				m_dRefreshGap = min(m_dRefreshGap, 1.0 / m_decoder.GetFrameRate() / 2);
				if (m_dRefreshGap < 0.01)
					m_dRefreshGap = 0.01;
			}
			m_pSync.reset(new CSyncVideoByAudioTime(1.0 / m_decoder.GetFrameRate(), m_dRefreshGap));
		}
		else
		{
			LARGE_INTEGER fi;
			fi.QuadPart = (LONGLONG)(1.0 / m_decoder.GetFrameRate()*m_liFreq.QuadPart);
			m_pSync.reset(new CSyncVideoByFrameRate(fi));
		}
		m_decoder.Init();
		m_iPlayStatue = 1;
	}

	return true;
}

void CMovieWindow::Pause()
{
	if (m_iPlayStatue == 0)
		return;
	if (1 == m_iPlayStatue)
	{
		if (m_decoder.HasAudio())
		{
			m_sound.Stop(true);
		}
		m_pSync->PausePlay();
		m_iPlayStatue = 2;
		//pause
	}
	else if (2==m_iPlayStatue)
	{
		if (m_decoder.HasAudio())
		{
			m_sound.Start();
		}
		m_pSync->PausePlay();
		m_iPlayStatue = 1;
		//resume
	}
}

__forceinline void CMovieWindow::MainLoop()
{
	int n;
	QueryPerformanceCounter(&m_liNow);
	if (m_liNow.QuadPart - m_liLast.QuadPart > m_liInterval.QuadPart)
	{
		m_liLast.QuadPart = m_liNow.QuadPart;

		Again:
		if (m_pCurrImage->ReadableBufferLen() < 1)
		{
			n = m_decoder.GetImageData(m_pCurrImage.get(), m_ImageInfo);
			if (n == 0)
			{
				if (m_ImageInfo.dataSize > m_pCurrImage->TotalBufferLen())
				{
					m_pCurrImage->Resize(m_ImageInfo.dataSize);
				}
			}
		}

		if(m_pCurrImage->ReadableBufferLen() > 0)
		{
			if (m_decoder.HasAudio())
			{
				m_pairForSyncAV.first = m_sound.PlayedTime();
				m_pairForSyncAV.second = m_ImageInfo.pts;
				n = m_pSync->IsSwitchToNextFrame(&m_pairForSyncAV);
			}
			else
			{
				n = m_pSync->IsSwitchToNextFrame(&m_liNow);
			}

			switch (n)
			{
			case ISyncVideo::DoShowThisFrameNow:
				m_pShow->UpdateImage(m_pCurrImage.get(), m_ImageInfo.width, m_ImageInfo.height);
				m_pShow->DrawScene();
				break;
			case ISyncVideo::DontShowThisFrameNow:
				break;
			case ISyncVideo::SkipThisFrame_ShowNext:
				m_pCurrImage->Reset();
				goto Again;
			}
		}
	}

	Sleep(1);
}

bool CMovieWindow::WriteAudioData()
{
	int n;
	if (m_pSoundBuf->ReadableBufferLen() < m_pSoundBuf->TotalBufferLen()/2)
	{
		n = m_decoder.GetAudioData(m_pSoundBuf.get(), m_pSoundBuf->WriteableBufferLen());
	}

	if (m_pSoundBuf->IsEmpty())
	{
		return false;
	}

	DWORD t1, t2;
	return 0 != m_sound.Write(m_pSoundBuf.get(), t1, t2);
}