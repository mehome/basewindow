#include "MovieWindow.h"

bool CMovieShow::Init()
{
	EnableCustomNCHitTest(false);

	CHLayout* pLayout = new CHLayout(this);
	pLayout->SetContentMargin(0, 0, 0, 0);

	CImageLayer* pImage = new CImageLayer();
	pImage->SetSizePolicy(SizePolicyExpanding);
	pImage->CreateImageLayerByColor(0, 0, 0);
	pLayout->AddChild(pImage);
	return CScene::Init();
}

CMovieWindow::CMovieWindow():
	m_iCountForAudio(0)
{
	QueryPerformanceFrequency(&m_liFreq);
}

CMovieWindow::~CMovieWindow()
{
}

LRESULT CMovieWindow::CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed)
{
	if (message == WM_MOVING || message==WM_SIZING)
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
		m_pDir->RunScene(new CMovieShow());

		//OpenFile("g:\\ËÀÊÌ.Deadpool.2016.BD-720p.1280X720.ÖÐÓ¢Ë«Óï-µç²¨×ÖÄ»×é.mp4");
		OpenFile("C:\\Users\\Think\\Desktop\\ÎÒµÄÒôÀÖ\\J'aimerais tellement.mp4");
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
	if (m_decoder.LoadFile(fileName))
	{
		m_decoder.ConfigureAudioOut();
		m_decoder.ConfigureVideoOut();

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
		return WriteAudioData();
	}

	return false;
}

__forceinline void CMovieWindow::MainLoop()
{
	QueryPerformanceCounter(&m_liNow);
	if (m_liNow.QuadPart - m_liLast.QuadPart > m_liInterval.QuadPart)
	{
		m_liLast.QuadPart = m_liNow.QuadPart;
		++m_iCountForAudio;
		if (m_iCountForAudio > 20)
		{
			m_iCountForAudio = 0;
			if (!WriteAudioData())
			{
				m_sound.Stop();
			}
		}
	}
	else
	{
		Sleep(1);
	}
}

bool CMovieWindow::WriteAudioData()
{
	int n;
	if (m_pSoundBuf->ReadableBufferLen() < m_pSoundBuf->TotalBufferLen()/2)
	{
		m_decoder.DecodeAudio(m_pSoundBuf.get(), n);
	}

	if (m_pSoundBuf->IsEmpty())
	{
		return false;
	}

	DWORD t1, t2;
	return 0 != m_sound.Write(m_pSoundBuf.get(), t1, t2);
}