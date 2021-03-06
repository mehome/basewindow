#include "MovieWindow.h"
#include <windowsx.h>

bool CMovieShow::Init()
{
	EnableCustomNCHitTest(false);

	m_pImage = new CImageLayer();
	m_pImage->SetSizePolicy(SizePolicyExpanding);
	m_pImage->CreateImageLayerByColor(0, 0, 0);
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
		m_pImage->CreateImageLayerByData((unsigned char*)p->Data(), w, h, 24, false, true);
	}
	else
	{
		memcpy(m_pImage->ImageData(), p->Data(), p->ReadableBufferLen());
	}
	p->Reset();
}

//void CMovieShow::CalculateRect()
//{
//	if (IsNeedUpdateRect())
//	{
//		CScene::CalculateRect();
//
//		auto size = GetSize();
//		float sx = size.first / m_pairFrameSize.first;
//		float sy = size.second / m_pairFrameSize.second;
//		sx = min(sx, sy);
//
//		sy = m_pairFrameSize.second * sx;
//		sx = m_pairFrameSize.first * sx;
//		m_pImage->SetSize(sx, sy);
//		m_pImage->SetPos(size.first / 2, size.second / 2);
//		auto r = m_pImage->GetRect();
//		m_pImage->ScaleImageInside(r.right - r.left, r.bottom - r.top);
//	}
//}

bool CMoveShow2D::Init()
{
	auto& size = GetSize();
	m_pImage = new CNode2DImageLayer(this);
	m_pImage->CreateImageLayerByColor(0, 0, 0);
	m_pImage->SetSize(size.first, size.second);
	m_pImage->SetPos(size.first/2, size.second/2);
	return CScene2D::Init();
}

void CMoveShow2D::DrawNode(DrawKit* pDrawKit)
{
	CScene2D::DrawNode(pDrawKit);
}

void CMoveShow2D::CalculateRect()
{
	if (IsNeedUpdateRect())
	{
		CScene2D::CalculateRect();

		auto size = GetSize();
		float sx = size.first / m_pairFrameSize.first;
		float sy = size.second / m_pairFrameSize.second;
		sx = min(sx, sy);

		sy = m_pairFrameSize.second * sx;
		sx = m_pairFrameSize.first * sx;
		m_pImage->SetSize(sx, sy);
		m_pImage->SetPos(size.first / 2, size.second / 2);
	}
}

void CMoveShow2D::UpdateImage(RingBuffer*p, int w, int h)
{
	auto& size = m_pImage->GetImageInfoSize();
	if (size.width != w || size.height != h)
	{
		m_pImage->CreateImageLayerByData((unsigned char*)p->Data(), w, h, 0);
	}
	else
	{
		m_pImage->UpdateImageData(p->Data());
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
		bool bPause(true);
		if (m_iPlayStatue == 0)
		{
			return 0;
		}
		else if (m_iPlayStatue == 1)
		{
			m_pSync->PausePlay(true);
			bPause = false;
		}

		int x = GET_X_LPARAM(lParam);
		if (m_decoder.HasAudio())
		{
			KillTimer(GetHWND(), (UINT_PTR)this);
			m_sound.Stop();

			m_decoder.SetSeekCallback([bPause, this]() {
				PostMessage(GetHWND(), WM_USER + 10, (WPARAM)bPause, 0);
				TRACE1("seek callback %d\n", rand()%5000);
			});

			m_decoder.SeekTime(m_decoder.GetDurationAll()*x / m_ImageInfo.width, m_sound.PlayedTime());
		}
		else
		{
			m_decoder.SetSeekCallback([bPause, this]() {
				PostMessage(GetHWND(), WM_USER + 10, (WPARAM)bPause, 0);
			});

			m_decoder.SeekTime(1.0*m_decoder.GetDurationAll()*x / m_ImageInfo.width, m_ImageInfo.pts);
		}
	}
	else if (message == WM_USER + 10)
	{
		bool bPause = (bool)wParam;
		if (m_decoder.HasAudio())
		{
			m_sound.Seek();
			m_sound.SetAudioBaseTime(m_decoder.AudioBaseTime());
			m_pCurrImage->Reset();
			m_pSoundBuf->Reset();
			m_sound.Start();
			m_iPlayStatue = 1;
			m_pSync->PausePlay(false);

			if (WriteAudioData())
			{
				SetTimer(GetHWND(), (UINT_PTR)this, 400, NULL);
				m_liLast.QuadPart = 0;
				MainLoop();
			}


			if (bPause)
			{
				Pause();
			}

		}
		else
		{
			m_iPlayStatue = 1;
			m_pSync->PausePlay(false);
			m_pCurrImage->Reset();
			m_liLast.QuadPart = 0;
			MainLoop();

			if (bPause)
			{
				Pause();
			}
		}
	}
	else if (message == WM_CREATE)
	{
		RECT r1, r2, area = GetAvaliableDestktopArea();
		GetWindowRect(hWnd, &r1);
		GetClientRect(hWnd, &r2);
		ReSize(min(area.right - area.left, 1280 + (r1.right - r1.left) - (r2.right - r2.left)),
			min(area.bottom - area.top, 720 + (r1.bottom - r1.top) - (r2.bottom - r2.top)),
			true);
		SetWindowText(GetHWND(), TEXT("Movie"));

		//CGDIView* pView = new CGDIView();
		Node2DView* pView = new Node2DView();
		pView->Init(GetHWND());
		m_pDir.reset(new CDirector(pView));
		//m_pShow = new CMovieShow();
		m_pShow = new CMoveShow2D();
		m_pDir->RunScene(m_pShow);

		//OpenFile("e:\\video\\BBCW_SavingOurSeabirds_1自然世界 拯救我们的海鸟.flv");
		//OpenFile("e:\\Prometheus.2012.普罗米修斯.国英双语.HR-HDTV.AC3.1024X576-人人影视制作.mkv");
		OpenFile("E:\\zhuozhou\\1\\安史之乱.flv");
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
	pIo->CreateBuffer(65536);
	m_decoder.SetCustomIOContext(pIo);
	if (m_decoder.LoadFile(fileName))
	{
		AudioParams outAudioParam;
		outAudioParam.sample_rate = m_decoder.GetSampleRate();
		m_decoder.ConfigureAudioOut(&outAudioParam);
		VideoParams outVideoParam;
		outVideoParam.width = m_decoder.GetFrameSize().first;
		outVideoParam.heigh = m_decoder.GetFrameSize().second;
		outVideoParam.fmt = AV_PIX_FMT_BGRA;
		m_decoder.ConfigureVideoOut(&outVideoParam);

		if (m_decoder.HasVideo())
		{
			m_decoder.DecodeVideo(NULL, m_ImageInfo);
			m_pCurrImage.reset(new RingBuffer(m_ImageInfo.dataSize));
			m_decoder.DecodeVideo(m_pCurrImage.get(), m_ImageInfo);
			m_pShow->SetFrameSize(m_decoder.GetFrameSize());
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

		if (!m_decoder.Init())
		{
			m_sound.Stop();
			m_sound.Clear();
			m_decoder.Clean();
			return false;
		}
		m_iPlayStatue = 1;
		return true;
	}

	return false;
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
		m_pSync->PausePlay(true);
		m_iPlayStatue = 2;
		//pause
	}
	else if (2==m_iPlayStatue)
	{
		if (m_decoder.HasAudio())
		{
			m_sound.Start();
		}
		m_pSync->PausePlay(false);
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