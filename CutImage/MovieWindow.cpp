#include "MovieWindow.h"

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
		if (!WriteAudioData() && m_sound.UnPlayedDataLen() < m_sound.SoundFormat().nAvgBytesPerSec*0.4)
		{
			m_sound.Stop();
			KillTimer(hWnd, (UINT_PTR)this);
		}
	}
	else if (message == WM_SIZING || message == WM_MOVING)
	{
		MainLoop();
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

		OpenFile("e:\\1.flv");
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
	m_liInterval.QuadPart = LONGLONG(20.0 / 1000 * m_liFreq.QuadPart);

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
	if (m_decoder.IsRunning())
	{
		m_decoder.Destroy();
	}
	m_decoder.Clean();
	m_sound.Stop();
	m_sound.Clear();
	KillTimer(GetHWND(), (UINT_PTR)this);

	if (m_decoder.LoadFile(fileName))
	{
		m_decoder.ConfigureAudioOut();
		m_decoder.ConfigureVideoOut();

		m_pCurrImage.reset(new RingBuffer(1080 * 720 * 4));

		if(m_decoder.HasAudio())
		{
			PCMWAVEFORMAT info;
			info.wBitsPerSample = 16;
			info.wf.nBlockAlign = 4;
			info.wf.nChannels = 2;
			info.wf.nSamplesPerSec = 44100;
			info.wf.wFormatTag = WAVE_FORMAT_PCM;
			info.wf.nAvgBytesPerSec = 44100 * 4;
			m_pSoundBuf.reset(new RingBuffer(info.wf.nAvgBytesPerSec));
			m_sound.Initialize(info.wf, info.wBitsPerSample, info.wf.nAvgBytesPerSec * 3, GetHWND());
			m_sound.Start();

			int n;
			DWORD t1, t2;
			m_decoder.DecodeAudio(m_pSoundBuf.get(), n);
			if (n > 0 && m_sound.Write(m_pSoundBuf.get(), t1, t2) != 0)
			{
				SetTimer(GetHWND(), (UINT_PTR)this, 400, NULL);
			}
			m_pSync.reset(new CSyncVideoByAudioTime(1.0/m_decoder.GetFrameRate(), 20.0/1000));
		}
		else
		{
			LARGE_INTEGER fi;
			fi.QuadPart = 1.0 / m_decoder.GetFrameRate()*m_liFreq.QuadPart;
			m_pSync.reset(new CSyncVideoByFrameRate(fi));
		}
		m_decoder.Init();
	}

	return false;
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
			case ISyncVideo::SkiThisFrame_ShowNext:
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