#include "Mp3Player.h"

#pragma comment(lib,"dsound.lib")
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
		TRACE("failed DirectSoundCreate\n");
		return false;
	}
	if (hWnd == NULL)
	{
		hWnd = GetDesktopWindow();
	}
	hr = lpDS_->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
	if (FAILED(hr))
	{
		TRACE("failed SetCooperativeLevel\n");
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
		TRACE("failed CreateSoundBuffer\n");
		Clear();
		if (DSERR_ALLOCATED == hr)
		{
			TRACE("DSERR_ALLOCATED:The request failed because resources, such as a priority level, were already in use by another caller. ");
		}
		else if (hr == DSERR_BADFORMAT)
		{
			TRACE("DSERR_BADFORMAT:The specified wave format is not supported.");
		}
		else if (hr == DSERR_BUFFERTOOSMALL)
		{
			TRACE("DSERR_BUFFERTOOSMALL:The buffer size is not great enough to enable effects processing.");
		}
		else if (hr == DSERR_CONTROLUNAVAIL)
		{
			TRACE("DSERR_CONTROLUNAVAIL:The buffer control (volume, pan, and so on) requested by the caller is not available.");
		}
		else if (hr == DSERR_DS8_REQUIRED)
		{
			TRACE("DSERR_DS8_REQUIRED:A DirectSound object of class CLSID_DirectSound8 or later is required for the requested functionality.");
		}
		else if (hr == DSERR_INVALIDCALL)
		{
			TRACE("DSERR_INVALIDCALL :This function is not valid for the current state of this object.");
		}
		else if (hr == DSERR_INVALIDPARAM)
		{
			TRACE("DSERR_INVALIDPARAM :An invalid parameter was passed to the returning function.");
		}
		else if (hr == DSERR_NOAGGREGATION)
		{
			TRACE("DSERR_NOAGGREGATION :The object does not support aggregation. ");
		}
		else if (hr == DSERR_OUTOFMEMORY)
		{
			TRACE("DSERR_OUTOFMEMORY :The DirectSound subsystem could not allocate sufficient memory to complete the caller's request. ");
		}
		else if (hr == DSERR_UNINITIALIZED)
		{
			TRACE("DSERR_UNINITIALIZED:Initialize method has not been called or has not been called successfully before other methods were called. ");
		}
		else if (hr == DSERR_UNSUPPORTED)
		{
			TRACE("DSERR_UNSUPPORTED The function called is not supported at this time. ");
		}
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
		if (hr == DS_OK)
		{
			ClearBuffer(0);
		}
		else
		{
			TRACE("failed Play\n");
			return FALSE;
		}
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
	//0error
	//1ok,buffer is full
	//2ok,
	//3try again

	int status = -1;
	DWORD dwWrite, dwPlay;
	HRESULT hr;
	unsigned char*pb1(NULL), *pb2(NULL);
	DWORD dwb1(0), dwb2(0);

	//iWritePos_为零代表第一次写缓冲区
	if (iWritePos_ == -1)
	{
		hr = lpDSB_->GetCurrentPosition(&dwPlay, &dwWrite);
		if (FAILED(hr))
		{
			/*if(hr==DSERR_BUFFERLOST)
			{
			hr=lpDSB_->Restore();
			if(hr!=DS_OK)
			{
			return 0;
			}
			}
			else
			{
			return 0;
			}*/
			return 0;
		}

		dwLockOffset_ = dwWrite;
		dwLockLen_ = __min(dwLen, dwBufferLength_);
	}
	else
	{
		status = AvaliableBuffer(dwLen, dwLockOffset_, dwLockLen_);
		if (status == 0 || status==1)
			return status;
	}
	//接下来要写缓冲区

	hr = lpDSB_->Lock(dwLockOffset_, dwLockLen_, (LPVOID*)&pb1, &dwb1, (LPVOID*)&pb2, &dwb2, 0);
	if (hr != DS_OK)
	{
		if (hr == DSERR_BUFFERLOST)
		{
			TRACE("CSound_Write:DSERR_BUFFERLOST");
			hr = lpDSB_->Restore();
			if (hr == DS_OK)
			{
				ClearBuffer(0);
				iWritePos_ = -1;
				return 3;
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

	//更新对有效缓冲区（正在播放）的记录
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
	return 2;
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
	if (samplePos >= bufferStart && samplePos <= int(dwBufferLength_))
	{
	}
	else if (samplePos >= 0 && samplePos < (bufferStart + bufferLen) % (int)dwBufferLength_)
	{
	}
	else
	{
		throw 1;
	}
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
unsigned int CSound::GetRemainderTime()const
{
	HRESULT hr;
	DWORD dwPlay(0), dwWrite(0);
	DWORD dwDataLen(0), dwTemp;
	unsigned int iTime(0);


	hr = lpDSB_->GetCurrentPosition(&dwPlay, &dwWrite);
	if (FAILED(hr))
	{
		TRACE("failed GetCurrentPosition\n");
		return iTime;
	}

	if (ulBufferPos_ + ulBufferLength_ < dwBufferLength_)
	{
		TRACE3("没有环回(bufferpos:%u-bufferlength:%u-playpos:%u\n", ulBufferPos_, ulBufferLength_, dwPlay);
		dwDataLen = ulBufferLength_ - (dwPlay - ulBufferPos_);
	}
	else//缓冲区环回
	{
		TRACE("缓冲区环回\n");
		dwTemp = ulBufferLength_ - (dwBufferLength_ - ulBufferPos_);
		//TRACE1("buffer end %u\n",dwTemp);
		if (dwPlay >= ulBufferPos_ && dwPlay < dwBufferLength_)
		{
			TRACE3("当前播放位置在缓冲区的后半段(bufferpos:%u-bufferlength:%u-playpos:%u\n", ulBufferPos_, ulBufferLength_, dwPlay);
			dwDataLen = ulBufferLength_ - (dwPlay - ulBufferPos_);
		}
		else if (dwPlay >= 0 && dwPlay <= dwTemp)
		{
			TRACE3("当前播放位置在缓冲区的前半段(bufferpos:%u-bufferlength:%u-playpos:%u\n", ulBufferPos_, ulBufferLength_, dwPlay);
			dwDataLen = dwTemp - dwPlay;
		}
		else
		{
			TRACE("已经没有尚未播放的声音数据了\n");
			dwDataLen = 0;
		}
	}

	iTime = static_cast<unsigned int>(1000.0*dwDataLen / waveFormat_.nAvgBytesPerSec);
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

	hr = this->lpDSB_->Lock(start, count, (LPVOID *)&pb1, &dwb1, (LPVOID*)&pb2, &dwb2, flags);
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
	//0error
	//1buffer is full,but ok
	//2all right

	HRESULT hr;
	DWORD dwPlay = 0, dwWrite = 0;
	DWORD dwTmp;

	hr = lpDSB_->GetCurrentPosition(&dwPlay, &dwWrite);
	if (FAILED(hr))
	{
		return 0;
	}

	if (ulBufferPos_ + ulBufferLength_ < dwBufferLength_)
	{
		//here dwPlay>=ulBufferPos_ && dwPlay<ulBufferPos_+ulBufferLength_
		ulBufferLength_ = ulBufferLength_ - (dwPlay - ulBufferPos_);
		ulBufferPos_ = dwPlay;

		dwRealWritePos = ulBufferPos_ + ulBufferLength_;
		dwAvaliableLength = __min(dwWant, dwBufferLength_ - ulBufferLength_);
	}
	//有效缓冲区环回了
	else
	{
		dwTmp = ulBufferLength_ - (dwBufferLength_ - ulBufferPos_);
		if (dwTmp == ulBufferPos_)
		{
			if (dwPlay >= ulBufferPos_ && dwPlay < dwBufferLength_)
			{
				ulBufferLength_ -= (dwPlay - ulBufferPos_);
				ulBufferPos_ = dwPlay;
			}
			else if (dwPlay >= 0 && dwPlay < dwTmp)
			{
				ulBufferPos_ = dwPlay;
				ulBufferLength_ = dwTmp - dwPlay;
			}
			//缓冲区是满的，无需加数据
			return 1;
		}

		if (dwPlay >= ulBufferPos_ && dwPlay < dwBufferLength_)
		{
			ulBufferLength_ -= (dwPlay - ulBufferPos_);
			ulBufferPos_ = dwPlay;
			dwRealWritePos = dwTmp;
			dwAvaliableLength = __min(dwWant, dwBufferLength_ - ulBufferLength_);
		}
		else if (dwPlay >= 0 && dwPlay <= dwTmp)
		{
			ulBufferLength_ = dwTmp - dwPlay;
			ulBufferPos_ = dwPlay;
			dwRealWritePos = dwTmp;
			dwAvaliableLength = __min(dwWant, dwBufferLength_ - ulBufferLength_);
		}
		else
		{
			TRACE("never here\n");
			dwRealWritePos = dwWrite;
			dwAvaliableLength = __min(dwWant, dwBufferLength_);
			ulBufferPos_ = dwRealWritePos;
			ulBufferLength_ = dwAvaliableLength;
		}
	}//loop buffer
	return 2;
}//

CWM::CWM()
{
	bOpened_ = false;
	pWMSyncReader_ = NULL;
	pHeaderInfo_ = NULL;
	pNSSBuffer_ = NULL;
	wstrName_ = NULL;

	memset(&pcmWF_, 0, sizeof(pcmWF_));
	bHasAudio_ = FALSE;
	dwOutput_ = 0;
	wStream_ = 0;
	bProtected_ = FALSE;

	dwLastRead_ = 0;
	liReadedDuration_.QuadPart = 0;
}
CWM::~CWM()
{
	TRACE("CWM Destructor\n");
	Clear();
}
void CWM::Clear()
{
	if (!bOpened_)
	{
		return;
	}

	if (wstrName_)
	{
		free(wstrName_);
		wstrName_ = NULL;
	}
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
	bOpened_ = false;
	::CoUninitialize();
}
bool CWM::Initialize(std::string szFileName, bool bDiscrete)
{
	DWORD dwi = 0, dwj = 0;
	DWORD dwOutputCount = 0, dwOutputFormatCount = 0;
	DWORD dwSize;
	HRESULT hr;
	IWMOutputMediaProps *pWMOutputMediaProps = NULL;
	WM_MEDIA_TYPE *pOutputMediaType = NULL;
	WM_MEDIA_TYPE *pFormatMediaType = NULL;
	PWAVEFORMATEX pWaveFormat = NULL;
	BYTE byteEnable = FALSE;
	BOOL bCond = FALSE;

	if (bOpened_)
	{
		return false;
	}

	int len = ::MultiByteToWideChar(CP_ACP, 0, szFileName.c_str(), szFileName.length(), NULL, 0);
	wstrName_ = (wchar_t*)malloc(sizeof(wchar_t)*(len + 1));
	::MultiByteToWideChar(CP_ACP, 0, szFileName.c_str(), szFileName.length(), wstrName_, len);
	wstrName_[len] = L'\0';

	hr = ::CoInitialize(NULL);
	if (hr == S_OK)
	{
		bOpened_ = true;
	}
	else if (hr == S_FALSE)
	{
		//The COM library is already initialized on this thread.
		bOpened_ = true;
	}
	hr = WMCreateSyncReader(NULL, 0, &pWMSyncReader_);
	if (hr != S_OK)
	{
		return false;
	}
	hr = pWMSyncReader_->Open(wstrName_);
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
		hr = pWMSyncReader_->GetOutputProps(dwi, &pWMOutputMediaProps);
		if (hr != S_OK)
			continue;
		hr = pWMOutputMediaProps->GetMediaType(NULL, &dwSize);
		if (hr != S_OK)
			continue;
		pOutputMediaType = (WM_MEDIA_TYPE*)malloc(dwSize);
		if (pOutputMediaType == NULL)
		{
			TRACE("out of memory\n");
			continue;
		}
		hr = pWMOutputMediaProps->GetMediaType(pOutputMediaType, &dwSize);
		if (hr != S_OK)
		{
			free(pOutputMediaType);
			pOutputMediaType = NULL;
			continue;
		}
		pWMOutputMediaProps->Release();
		pWMOutputMediaProps = NULL;

		if (::IsEqualGUID(pOutputMediaType->majortype, WMMEDIATYPE_Audio))
		{
			byteEnable = static_cast<BYTE>(bDiscrete);
			pWMSyncReader_->SetOutputSetting(dwi, ::g_wszEnableDiscreteOutput,
				WMT_TYPE_BOOL, &byteEnable, 4);

			pWaveFormat = (PWAVEFORMATEX)pOutputMediaType->pbFormat;
			pcmWF_.wf.nSamplesPerSec = pWaveFormat->nSamplesPerSec;
			pcmWF_.wf.nChannels = pWaveFormat->nChannels;

			hr = pWMSyncReader_->GetOutputFormatCount(dwi, &dwOutputFormatCount);
			if (hr == S_OK)
			{
				for (dwj = 0; dwj<dwOutputFormatCount; ++dwj)
				{
					hr = pWMSyncReader_->GetOutputFormat(dwi, dwj, &pWMOutputMediaProps);
					if (hr != S_OK)
						continue;
					hr = pWMOutputMediaProps->GetMediaType(NULL, &dwSize);
					if (hr != S_OK)
						continue;
					pFormatMediaType = (WM_MEDIA_TYPE*)malloc(dwSize);
					if (pFormatMediaType == NULL)
					{
						TRACE("out of memory");
						continue;
					}
					hr = pWMOutputMediaProps->GetMediaType(pFormatMediaType, &dwSize);
					if (hr != S_OK)
					{
						free(pFormatMediaType);
						pFormatMediaType = NULL;
						continue;
					}

					if (::IsEqualGUID(pFormatMediaType->formattype, WMFORMAT_WaveFormatEx))
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
							pWMSyncReader_->SetOutputProps(dwi, pWMOutputMediaProps);
						}
					}
					free(pFormatMediaType);
					pFormatMediaType = NULL;
				}//dwj
			}
		}//IsEqualGUID
		free(pOutputMediaType);
		pOutputMediaType = NULL;

		pWMOutputMediaProps->Release();
		pWMOutputMediaProps = NULL;
	}//for dwi

	if (bHasAudio_)
	{
		pWMSyncReader_->GetStreamNumberForOutput(dwOutput_, &wStream_);
		if (pWMSyncReader_->SetReadStreamSamples(wStream_, FALSE) == NS_E_PROTECTED_CONTENT)
		{
			bProtected_ = FALSE;
		}
		bProtected_ = TRUE;
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

		//wStream_=0;
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
	bool b = Initialize(szName);

	//if (b)
	//{
	//	szFileName_ = szName;
	//}
	return b;
}
bool CWM::IsValid()
{
	return true;
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

	if (!bOpened_)
	{
		return 0;
	}
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
			//pNSSBuffer_->Release();
			//pNSSBuffer_=NULL;
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
	pcmWF_.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmWF_.wf.nBlockAlign = pcmWF_.wBitsPerSample*pcmWF_.wf.nChannels / 8;
	pcmWF_.wf.nAvgBytesPerSec = pcmWF_.wf.nSamplesPerSec*pcmWF_.wf.nBlockAlign;
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