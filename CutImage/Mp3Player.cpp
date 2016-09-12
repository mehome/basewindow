#include "Mp3Player.h"

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "wmvcore.lib")

CSound::CSound()
{
	bPlaying_ = false;
	lpDS_ = NULL;
	lpDSB_ = NULL;
	dwBufferLength_ = 0;
	dwSilenceBytes_ = 0;

	iWritePos_ = -1;
	ulBufferPos_ = 0;
	ulBufferLength_ = 0;

	dwLockOffset_ = 0;
	dwLockLen_ = 0;
}

CSound::~CSound()
{
	if (bPlaying_)
	{
		Stop();
	}
	Clear();
}

bool CSound::Initialize(WAVEFORMAT wf, WORD wBitsPerSample, DWORD dwBufferLen, HWND hWnd)
{
	HRESULT hr;
	DSBUFFERDESC dsbd;

	Clear();
	hr = DirectSoundCreate(NULL, &lpDS_, NULL);
	if (FAILED(hr))
	{
		return false;
	}
	if (hWnd == NULL)
	{
		hWnd = GetDesktopWindow();
	}
	hr = lpDS_->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
	if (FAILED(hr))
	{
		Clear();
		return FALSE;
	}

	waveFormat_.nAvgBytesPerSec = wf.nAvgBytesPerSec;
	waveFormat_.nBlockAlign = wf.nBlockAlign;
	waveFormat_.nChannels = wf.nChannels;
	waveFormat_.nSamplesPerSec = wf.nSamplesPerSec;
	waveFormat_.wFormatTag = wf.wFormatTag;
	waveFormat_.wBitsPerSample = wBitsPerSample;

	dwBufferLength_ = dwBufferLen;
	memset(&dsbd, 0, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.lpwfxFormat = (WAVEFORMATEX*)&waveFormat_;
	dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
	dsbd.dwBufferBytes = dwBufferLen;

	hr = lpDS_->CreateSoundBuffer(&dsbd, &lpDSB_, NULL);
	if (FAILED(hr))
	{
		Clear();
		return false;
	}
	return true;
}

bool CSound::Clear()
{
	if (lpDSB_)
	{
		lpDSB_->Release();
		lpDSB_ -= NULL;
	}
	if (lpDS_)
	{
		lpDS_->Release();
		lpDS_ = NULL;
	}
	dwBufferLength_ = 0;
	dwSilenceBytes_ = 0;
	ulBufferPos_ = 0;
	iWritePos_ = -1;

	return true;
}

int CSound::Start()
{
	HRESULT hr;
	DWORD status;

	if (!lpDSB_)
		return FALSE;

	hr = lpDSB_->GetStatus(&status);
	if (hr == DS_OK && status == DSBSTATUS_LOOPING)
		return TRUE;

	hr = lpDSB_->Play(0, 0, DSCBSTART_LOOPING);
	if (hr == DSERR_BUFFERLOST)
	{
		hr = lpDSB_->Restore();
		if (FAILED(hr))
			return FALSE;
	}
	ClearBuffer(0);
	bPlaying_ = true;
	return TRUE;
}

void CSound::Stop()
{
	if (lpDSB_ && bPlaying_)
	{
		lpDSB_->Stop();
		bPlaying_ = FALSE;
	}
}

DWORD CSound::BufferLength()const
{
	return dwBufferLength_;
}

WAVEFORMATEX CSound::SoundFormat()const
{
	return waveFormat_;
}

int CSound::Write(void *pData, DWORD dwLen, DWORD &dwWritten)
{
	//0 error
	//1 ok,
	//2 try again

	int status(0);
	DWORD dwWrite, dwPlay;
	HRESULT hr;
	unsigned char*pb1(NULL), *pb2(NULL);
	DWORD dwb1(0), dwb2(0);

	//iWritePos_为-1代表第一次写缓冲区
	if (iWritePos_ == -1)
	{
		hr = lpDSB_->GetCurrentPosition(&dwPlay, &dwWrite);
		if (FAILED(hr))
		{
			return 0;
		}

		dwLockOffset_ = dwWrite;
		dwLockLen_ = __min(dwLen, dwBufferLength_);
	}
	else
	{
		status = AvaliableBuffer(dwLen, dwLockOffset_, dwLockLen_);
		if (status == 0)
			return status;
	}

	hr = lpDSB_->Lock(dwLockOffset_, dwLockLen_, (LPVOID*)&pb1, &dwb1, (LPVOID*)&pb2, &dwb2, 0);
	if (hr != DS_OK)
	{
		if (hr == DSERR_BUFFERLOST)
		{
			hr = lpDSB_->Restore();
			if (hr == DS_OK)
			{
				iWritePos_ = -1;
				return 2;
			}
		}
		return 0;
	}

	if (pb1 != NULL)
	{
		memcpy(pb1, pData, dwb1);
	}
	if (pb2 != NULL)
	{
		memcpy(pb2, (unsigned char*)pData + dwb1, dwb2);
	}
	lpDSB_->Unlock(pb1, dwb1, pb2, dwb2);

	//更新对有效缓冲区的记录
	if (iWritePos_ == -1)
	{
		iWritePos_ = (int)(dwLockOffset_ + dwb1 + dwb2);
		ulBufferPos_ = dwLockLen_;
		ulBufferLength_ = dwb1 + dwb2;
	}
	else
	{
		iWritePos_ += (dwb1 + dwb2);
		ulBufferLength_ += (dwb1 + dwb2);
	}
	iWritePos_ %= dwBufferLength_;

	dwWritten = dwb1 + dwb2;
	return 1;
}

void CSound::Seek()
{
	iWritePos_ = -1;
	ulBufferLength_ = 0;
	ulBufferPos_ = 0;
}

void CSound::SamplePosition(int &samplePos, int &bufferStart, int bufferLen)
{
	DWORD dwPlay;
	HRESULT hr;

	if (ulBufferLength_ < 1)
	{
		samplePos = 1;
		return;
	}

	hr = lpDSB_->GetCurrentPosition(&dwPlay, NULL);
	if (FAILED(hr))
	{
		samplePos = -1;
		return;
	}

	samplePos = static_cast<int>(dwPlay);
	bufferStart = static_cast<int>(ulBufferPos_);
	bufferLen = static_cast<int>(ulBufferLength_);
}

void CSound::SamplePosition(int &samplePos)
{
	DWORD dwPlay;
	HRESULT hr;

	if (ulBufferLength_ < 1)
	{
		samplePos = 1;
		return;
	}

	hr = lpDSB_->GetCurrentPosition(&dwPlay, NULL);
	if (FAILED(hr))
	{
		samplePos = -1;
		return;
	}

	samplePos = static_cast<int>(dwPlay);
}

void CSound::CopyPosition(int &pos, int &length)
{
	pos = static_cast<int>(dwLockOffset_);
	length = static_cast<int>(dwLockLen_);
}

unsigned int CSound::GetRemainderTime()
{
	DWORD dwLockLen(0), dwLockPos(0);
	unsigned int iTime(0);

	if(AvaliableBuffer((DWORD)10000, dwLockPos, dwLockLen) != 0)
	{
		iTime = static_cast<unsigned int>(1000.0*ulBufferLength_ / waveFormat_.nAvgBytesPerSec);
	}

	return iTime;
}

void CSound::ClearBuffer(int type)
{
	unsigned char *pb1 = NULL;
	unsigned char *pb2 = NULL;
	DWORD dwb1 = 0;
	DWORD dwb2 = 0;
	DWORD flags = 0;
	DWORD start, count;
	HRESULT hr;

	if (type == 0)
	{
		flags |= DSBLOCK_ENTIREBUFFER;
		start = 0;
		count = dwBufferLength_;
	}
	else
	{
		hr = lpDSB_->GetCurrentPosition(&dwb1, &dwb2);
		if (FAILED(hr))
		{
			return;
		}
		start = dwb2;
		if (dwb2 >= dwb1)
		{
			count = dwBufferLength_ - dwb2 + dwb1;
		}
		else
		{
			count = dwb1 - dwb2;
		}
	}
	dwSilenceBytes_ = count;

	hr = this->lpDSB_->Lock(start, count, (void**)&pb1, &dwb1, (void**)&pb2, &dwb2, flags);
	if (FAILED(hr))
	{
		return;
	}
	TRACE2("CSound_ClearBuffer:offset:%ld,length:%ld\n", start, count);
	if (pb1 != NULL)
	{
		memset(pb1, waveFormat_.wBitsPerSample == 8 ? 128 : 0, dwb1);
	}
	if (pb2 != NULL)
	{
		memset(pb2, waveFormat_.wBitsPerSample == 8 ? 128 : 0, dwb2);
	}
	lpDSB_->Unlock(pb1, dwb1, pb2, dwb2);
}

int CSound::AvaliableBuffer(DWORD dwWant, DWORD &dwRealWritePos, DWORD &dwAvaliableLength)
{
	//about return
	//0 error
	//1 ok

	HRESULT hr;
	DWORD dwPlay(0), dwWrite(0), dwTmp;
	DWORD ulBufferEnd;

	hr = lpDSB_->GetCurrentPosition(&dwPlay, &dwWrite);
	if (FAILED(hr))
		return 0;

	ulBufferEnd = ulBufferPos_ + ulBufferLength_;
	if (ulBufferEnd < dwBufferLength_)
	{
		if(dwPlay >= ulBufferPos_ && dwPlay <ulBufferEnd)
		{
			ulBufferPos_ = dwPlay;
			ulBufferLength_ = ulBufferEnd - dwPlay;
			dwRealWritePos = ulBufferEnd;
			TRACE("A\n");
		}
		else
		{
			ulBufferPos_ = dwPlay;
			ulBufferLength_ = max(0, dwWrite - dwPlay);
			dwRealWritePos = dwWrite;
			TRACE("b\n");
		}
	}
	//有效缓冲区环回了
	else
	{
		dwTmp = ulBufferEnd - dwBufferLength_;
		if (dwPlay >= ulBufferPos_ && dwPlay < dwBufferLength_)
		{
			ulBufferLength_ -= (dwPlay - ulBufferPos_);
			ulBufferPos_ = dwPlay;
			dwRealWritePos = dwTmp;
			TRACE("C\n");
		}
		else if (dwPlay >= 0 && dwPlay < dwTmp)
		{
			ulBufferLength_ = dwTmp - dwPlay;
			ulBufferPos_ = dwPlay;
			dwRealWritePos = dwTmp;
			TRACE("D\n");
		}
		else
		{
			ulBufferPos_ = dwPlay;
			ulBufferLength_ = max(0, dwWrite - dwPlay);
			dwRealWritePos = dwWrite;
			TRACE("e\n");
		}
	}
	dwAvaliableLength = min(dwWant, dwBufferLength_ - ulBufferLength_);

	return 1;
}

CWM::CWM()
{
	pWMSyncReader_ = NULL;
	pHeaderInfo_ = NULL;
	pNSSBuffer_ = NULL;

	memset(&pcmWF_, 0, sizeof(pcmWF_));
	bHasAudio_ = FALSE;
	dwOutput_ = 0;
	wStream_ = 0;

	dwLastRead_ = 0;
	liReadedDuration_.QuadPart = 0;
}

CWM::~CWM()
{
	Clear();
}

void CWM::Clear()
{
	if (pNSSBuffer_)
	{
		pNSSBuffer_->Release();
		pNSSBuffer_ = NULL;
	}
	if (pHeaderInfo_)
	{
		pHeaderInfo_->Release();
		pHeaderInfo_ = NULL;
	}
	if (pWMSyncReader_)
	{
		pWMSyncReader_->Release();
		pWMSyncReader_ = NULL;
	}
}
bool CWM::Initialize(std::string szFileName, bool bDiscrete)
{
	DWORD dwi = 0, dwj = 0, dwSize(0);
	DWORD dwOutputCount = 0, dwOutputFormatCount = 0;
	HRESULT hr;
	IWMOutputMediaProps *pWMOutputMediaProps = NULL;
	WM_MEDIA_TYPE *pOutputMediaType = NULL;
	WM_MEDIA_TYPE *pFormatMediaType = NULL;
	PWAVEFORMATEX pWaveFormat = NULL;
	BYTE byteEnable = FALSE;
	BOOL bCond = FALSE;
	char buf[2048]={0};

	int len = MultiByteToWideChar(CP_ACP, 0, szFileName.c_str(), szFileName.length(), NULL, 0);
	std::unique_ptr<wchar_t[]> pWFileName(new wchar_t[len+1]);
	MultiByteToWideChar(CP_ACP, 0, szFileName.c_str(), szFileName.length(), pWFileName.get(), len);
	pWFileName[len]=0;

	hr = WMCreateSyncReader(NULL, 0, &pWMSyncReader_);
	if (hr != S_OK)
	{
		return false;
	}
	hr = pWMSyncReader_->Open(pWFileName.get());
	if (hr != S_OK)
	{
		pWMSyncReader_->Release();
		pWMSyncReader_ = NULL;
		return false;
	}
	hr = pWMSyncReader_->GetOutputCount(&dwOutputCount);
	if (hr != S_OK)
	{
		pWMSyncReader_->Release();
		pWMSyncReader_ = NULL;
		return false;
	}

	for (dwi = 0; dwi<dwOutputCount; ++dwi)
	{
		if(pWMOutputMediaProps)
		{
			pWMOutputMediaProps->Release();
			pWMOutputMediaProps=NULL;
		}
		hr = pWMSyncReader_->GetOutputProps(dwi, &pWMOutputMediaProps);
		if (hr != S_OK)
			continue;

		dwSize=2048;
		hr = pWMOutputMediaProps->GetMediaType((WM_MEDIA_TYPE*)buf, &dwSize);
		if (hr != S_OK)
			continue;

		pOutputMediaType=(WM_MEDIA_TYPE*)buf;
		if (!IsEqualGUID(pOutputMediaType->majortype, WMMEDIATYPE_Audio))
			continue;

		byteEnable = static_cast<BYTE>(bDiscrete);
		pWMSyncReader_->SetOutputSetting(dwi, g_wszEnableDiscreteOutput, WMT_TYPE_BOOL, &byteEnable, 4);

		pWaveFormat = (PWAVEFORMATEX)pOutputMediaType->pbFormat;
		pcmWF_.wf.nSamplesPerSec = pWaveFormat->nSamplesPerSec;
		pcmWF_.wf.nChannels = pWaveFormat->nChannels;

		hr = pWMSyncReader_->GetOutputFormatCount(dwi, &dwOutputFormatCount);
		if (hr != S_OK)
			continue;

		for (dwj = 0; dwj<dwOutputFormatCount; ++dwj)
		{
			if(pWMOutputMediaProps)
			{
				pWMOutputMediaProps->Release();
				pWMOutputMediaProps=NULL;
			}

			hr = pWMSyncReader_->GetOutputFormat(dwi, dwj, &pWMOutputMediaProps);
			if (hr != S_OK)
				continue;
					
			dwSize=2048;
			hr = pWMOutputMediaProps->GetMediaType((WM_MEDIA_TYPE*)buf, &dwSize);
			if (hr != S_OK)
				continue;
			pFormatMediaType=(WM_MEDIA_TYPE*)buf;
			if (IsEqualGUID(pFormatMediaType->formattype, WMFORMAT_WaveFormatEx))
			{
				pWaveFormat = (PWAVEFORMATEX)pFormatMediaType->pbFormat;
				bCond = (pWaveFormat->nSamplesPerSec == pcmWF_.wf.nSamplesPerSec) &&
					(pWaveFormat->nChannels == pcmWF_.wf.nChannels) &&
					(pWaveFormat->wBitsPerSample >= pcmWF_.wBitsPerSample);
				if (bCond)
				{
					bHasAudio_ = TRUE;
					dwOutput_ = dwi;
					pcmWF_.wBitsPerSample = pWaveFormat->wBitsPerSample;
					pcmWF_.wf.wFormatTag = WAVE_FORMAT_PCM;
					pcmWF_.wf.nBlockAlign = pcmWF_.wBitsPerSample*pcmWF_.wf.nChannels / 8;
					pcmWF_.wf.nAvgBytesPerSec = pcmWF_.wf.nSamplesPerSec*pcmWF_.wf.nBlockAlign;
					break;
				}
			}
		}
	}
	if(pWMOutputMediaProps)
		pWMOutputMediaProps->Release();

	if (bHasAudio_)
	{
		pWMSyncReader_->GetStreamNumberForOutput(dwOutput_, &wStream_);
		pWMSyncReader_->SetReadStreamSamples(wStream_, FALSE);

		WORD wLen = 8;
		WORD wAnyStream = 0;
		WMT_ATTR_DATATYPE dataType;
		hr = pWMSyncReader_->QueryInterface(IID_IWMHeaderInfo, (VOID**)&pHeaderInfo_);
		if (hr == S_OK)
		{
			hr = pHeaderInfo_->GetAttributeByName(&wAnyStream, L"Duration",
				&dataType,
				(BYTE*)&liDuration_,
				&wLen);
			if (hr != S_OK)
			{
				liDuration_.QuadPart = 0;
			}
		}
		else
		{
			this->Clear();
			return false;
		}

		//wchar_t *wstrTitle=NULL;
		//pHeaderInfo_->GetAttributeByName(&wStream_,L"Title",&dataType,NULL,&wLen);
		//wstrTitle=new wchar_t[wLen];
		//pHeaderInfo_->GetAttributeByName(&wStream_,L"Title",&dataType,(BYTE*)wstrTitle,&wLen);
		//delete[]wstrTitle;
	}

	return true;
}

bool CWM::SetFileName(std::string szName)
{
	return Initialize(szName, true);
}

int CWM::OutputData(char *pDest, int iWantLen)
{
	BYTE *pBuf;
	DWORD buflen;
	HRESULT hr;
	ULARGE_INTEGER sampleTime, duration;
	DWORD dwFlag;
	DWORD dwOutputNum;
	WORD wStream;

	int len = iWantLen;
	int dwPos = 0;
	int copyLen;

	while (len > 0)
	{
		if (pNSSBuffer_ != NULL)
		{
			pNSSBuffer_->GetBufferAndLength(&pBuf, &buflen);
			if (buflen == 0)
			{
				return iWantLen - len;
			}

			if ((DWORD)len <= buflen - dwLastRead_)
			{
				copyLen = len;
				memcpy(pDest + dwPos, pBuf + dwLastRead_, copyLen);
				dwLastRead_ += copyLen;
				len -= copyLen;
				return iWantLen - len;
			}
			else
			{
				copyLen = buflen - dwLastRead_;
				memcpy(pDest + dwPos, pBuf + dwLastRead_, copyLen);
				len -= copyLen;
				dwPos += copyLen;
			}

			pNSSBuffer_->Release();
			pNSSBuffer_ = NULL;
			dwLastRead_ = 0;
		}
		hr = pWMSyncReader_->GetNextSample(wStream_, &pNSSBuffer_,
			&sampleTime.QuadPart,
			&duration.QuadPart,
			&dwFlag,
			&dwOutputNum,
			&wStream);
		if (hr != S_OK)
		{
			return iWantLen - len;
		}
		else
		{
			liReadedDuration_.QuadPart += duration.QuadPart;
		}
	}

	return iWantLen - len;
}

PCMWAVEFORMAT CWM::SoundInfo()
{
	return this->pcmWF_;
}

void CWM::Seek(unsigned int uiMS)
{
	HRESULT hr;
	long long ms = static_cast<long long>(uiMS);

	ms *= 10000;
	hr = pWMSyncReader_->SetRange(ms, 0);
	if (FAILED(hr))
	{
		if (hr == E_INVALIDARG)
		{
			TRACE("E_INVALIDARG\n");
		}
		else if (hr == E_OUTOFMEMORY)
		{
			TRACE("E_OUTOFMEMORY\n");
		}
		else if (hr == E_UNEXPECTED)
		{
			TRACE("E_UNEXPECTED\n");
		}
		else
		{
			DWORD dw = ::GetLastError();
			TRACE1("CWMIn failed to seek.error code :%ld\n", dw);
		}
		TRACE("Error in seeking wm file\n");
	}
	else
	{
		if (pNSSBuffer_)
		{
			pNSSBuffer_->Release();
			pNSSBuffer_ = NULL;
		}
		dwLastRead_ = 0;
		liReadedDuration_.QuadPart = ms;
	}
}

unsigned int CWM::GetSoundLength()
{
	return static_cast<unsigned int>(liDuration_.QuadPart) / 10000;
}

unsigned int CWM::GetCurrentPos()
{
	return static_cast<unsigned int>(liReadedDuration_.QuadPart) / 10000;
}

CWM* p;
CSound *pSound;
char buf[176400];
const int clen=176400;
DWORD buflen=0;
LRESULT CMp3PlayerWindow::CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed)
{
	if(message == WM_CREATE)
	{
		p=new CWM();
		p->Initialize("d:\\3.mp4", true);

		pSound=new CSound();
		pSound->Initialize(p->SoundInfo().wf, p->SoundInfo().wBitsPerSample, p->SoundInfo().wf.nAvgBytesPerSec, hWnd);
		pSound->Start();

		SetTimer(hWnd, 0, 800, NULL);
	}
	else if(message == WM_TIMER)
	{
		int len=p->OutputData(buf+buflen, clen-buflen);
		buflen+=len;
		DWORD written;
		auto res=pSound->Write(buf, buflen, written);
		if(res == 1)
		{
			memcpy(buf, buf+written, buflen - written);
			buflen -=written;
		}
	}
	return CBaseWindow::CustomProc(hWnd, message, wParam, lParam, bProcessed);
}