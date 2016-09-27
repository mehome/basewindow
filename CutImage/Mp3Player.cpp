#include "Mp3Player.h"
#include <process.h>

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "wmvcore.lib")

#ifndef PI
#define PI_2 6.283185f
#define PI   3.1415925f
#endif

CSound::CSound()
{
	bPlaying_=false;
	lpDS_=NULL;
	lpDSB_=NULL;
	dwBufferLength_=0;
	dwSilenceBytes_=0;

	iWritePos_=-1;
	ulBufferPos_=0;
	ulBufferLength_=0;

	dwLockOffset_=0;
	dwLockLen_=0;
}

CSound::~CSound()
{
	if(bPlaying_)
	{
		Stop();
	}
	Clear();
}

bool CSound::Initialize(WAVEFORMAT wf,WORD wBitsPerSample,DWORD dwBufferLen,HWND hWnd)
{
	HRESULT hr;
	DSBUFFERDESC dsbd;

	Clear();
	hr=DirectSoundCreate(NULL,&lpDS_,NULL);
	if(FAILED(hr))
	{
		TRACE("failed DirectSoundCreate\n");
		return false;
	}
	if(hWnd==NULL)
	{
		hWnd=GetDesktopWindow();
	}
	hr=lpDS_->SetCooperativeLevel(hWnd,DSSCL_PRIORITY);
	if(FAILED(hr))
	{
		TRACE("failed SetCooperativeLevel\n");
		Clear();
		return FALSE;
	}

	waveFormat_.nAvgBytesPerSec=wf.nAvgBytesPerSec;
	waveFormat_.nBlockAlign=wf.nBlockAlign;
	waveFormat_.nChannels=wf.nChannels;
	waveFormat_.nSamplesPerSec=wf.nSamplesPerSec;
	waveFormat_.wFormatTag=wf.wFormatTag;
	waveFormat_.wBitsPerSample=wBitsPerSample;

	dwBufferLength_=dwBufferLen;
	memset(&dsbd,0,sizeof(DSBUFFERDESC));
	dsbd.dwSize=sizeof(DSBUFFERDESC);
	dsbd.lpwfxFormat=(WAVEFORMATEX*)&waveFormat_;
	dsbd.dwFlags=DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
	dsbd.dwBufferBytes=dwBufferLen;

	hr=lpDS_->CreateSoundBuffer(&dsbd,&lpDSB_,NULL);
	if(FAILED(hr))
	{
		TRACE("failed CreateSoundBuffer\n");
		Clear();
		return false;
	}
	return true;
}

bool CSound::Clear()
{
	if(lpDSB_)
	{
		lpDSB_->Release();
		lpDSB_-=NULL;
	}
	if(lpDS_)
	{
		lpDS_->Release();
		lpDS_=NULL;
	}
	dwBufferLength_=0;
	dwSilenceBytes_=0;
	ulBufferPos_=0;
	iWritePos_=-1;

	return true;
}

int CSound::Start()
{
	HRESULT hr;
	DWORD status;

	hr=lpDSB_->GetStatus(&status);
	if(hr==DS_OK && status==DSBSTATUS_LOOPING)
	{
		return TRUE;
	}

	hr=lpDSB_->Play(0,0,DSCBSTART_LOOPING);
	if(hr==DSERR_BUFFERLOST)
	{
		hr=lpDSB_->Restore();
		if(FAILED(hr))
			return FALSE;
	}
	ClearBuffer(0);
	bPlaying_=true;
	return TRUE;
}

void CSound::Stop()
{
	if(lpDSB_ && bPlaying_)
	{
		lpDSB_->Stop();
		bPlaying_=FALSE;
	}
}

const WAVEFORMATEX& CSound::SoundFormat()const
{
	return waveFormat_;
}

int CSound::Write(void *pData,DWORD dwLen,DWORD &dwWriteLen, DWORD& dwWritePos)
{
	//0error
	//1ok,buffer is full
	//2ok,
	//3try again

	int status(0);
	DWORD dwWrite,dwPlay;
	HRESULT hr;
	unsigned char*pb1,*pb2;
	DWORD dwb1,dwb2;

	//iWritePos_为零代表第一次写缓冲区
	if(iWritePos_ == -1)
	{
		hr=lpDSB_->GetCurrentPosition(&dwPlay,&dwWrite);
		if(FAILED(hr))
		{
			return 0;
		}

		dwLockOffset_=dwWrite;
		dwLockLen_=min(dwLen,dwBufferLength_);
	}
	else
	{
		status=AvaliableBuffer(dwLen,dwLockOffset_,dwLockLen_);
		if(status==0 || status==1)
		{
			return status;
		}
	}

	hr=lpDSB_->Lock(dwLockOffset_, dwLockLen_, (LPVOID*)&pb1, &dwb1, (LPVOID*)&pb2, &dwb2, 0);
	if(hr!=DS_OK)
	{
		if(hr=DSERR_BUFFERLOST)
		{
			hr=lpDSB_->Restore();
			if(hr==DS_OK)
			{
				ClearBuffer(0);
				iWritePos_=-1;
				return 3;
			}
		}
		return 0;
	}

	if(pb1 != NULL)
	{
		memcpy(pb1,pData,dwb1);
	}
	if(pb2 != NULL)
	{
		memcpy(pb2,(unsigned char*)pData+dwb1,dwb2);
	}
	lpDSB_->Unlock(pb1,dwb1,pb2,dwb2);

	//更新对有效缓冲区（正在播放）的记录
	if(iWritePos_ == -1)
	{
		iWritePos_ = (int)(dwLockOffset_+dwb1+dwb2);

		ulBufferPos_=dwLockOffset_;
		ulBufferLength_ = dwb1+dwb2;
	}
	else
	{
		iWritePos_ += int(dwb1+dwb2);
		ulBufferLength_ += (dwb1+dwb2);
	}
	iWritePos_ %=dwBufferLength_;

	dwWritePos = dwLockOffset_;
	dwWriteLen = dwLockLen_;
	return 2;
}
void CSound::Seek()
{
	iWritePos_=-1;
	ulBufferLength_=0;
	ulBufferPos_=0;
}

bool CSound::SamplePosition(int &samplePos)
{
	DWORD dwPlay;
	HRESULT hr;

	if(ulBufferLength_ < 1)
	{
		return false;
	}

	hr=lpDSB_->GetCurrentPosition(&dwPlay,NULL);
	if(FAILED(hr))
	{
		return false;
	}

	samplePos=static_cast<int>(dwPlay);
	return true;
}

void CSound::ClearBuffer(int type)
{
	unsigned char *pb1=NULL;
	unsigned char *pb2=NULL;
	DWORD dwb1=0;
	DWORD dwb2=0;
	DWORD flags=0;
	DWORD start,count;
	HRESULT hr;

	if(type==0)
	{
		flags|=DSBLOCK_ENTIREBUFFER;
		start=0;
		count=dwBufferLength_;
	}
	else
	{
		hr=lpDSB_->GetCurrentPosition(&dwb1,&dwb2);
		if(FAILED(hr))
		{
			return;
		}
		start=dwb2;
		if(dwb2>=dwb1)
		{
			count=dwBufferLength_-dwb2+dwb1;
		}
		else
		{
			count=dwb1-dwb2;
		}
	}
	dwSilenceBytes_=count;

	hr=this->lpDSB_->Lock(start,count,(LPVOID *)&pb1,&dwb1,(LPVOID*)&pb2,&dwb2,flags);
	if(FAILED(hr))
	{
		return;
	}
	TRACE2("CSound_ClearBuffer:offset:%ld,length:%ld\n",start,count);
	if(pb1!=NULL)
	{
		memset(pb1,waveFormat_.wBitsPerSample==8?128:0,dwb1);
	}
	if(pb2!=NULL)
	{
		memset(pb2,waveFormat_.wBitsPerSample==8?128:0,dwb2);
	}
	lpDSB_->Unlock(pb1,dwb1,pb2,dwb2);

}
int CSound::AvaliableBuffer(DWORD dwWant,DWORD &dwRealWritePos,DWORD &dwAvaliableLength)
{
	//about return
	//0error
	//1buffer is full,but ok
	//2all right

	HRESULT hr;
	DWORD dwPlay=0,dwWrite=0;
	DWORD dwTmp;

	hr=lpDSB_->GetCurrentPosition(&dwPlay,&dwWrite);
	if(FAILED(hr))
	{
		return 0;
	}

	if(ulBufferPos_+ulBufferLength_ < dwBufferLength_)
	{
		//here dwPlay>=ulBufferPos_ && dwPlay<ulBufferPos_+ulBufferLength_
		ulBufferLength_=ulBufferLength_-(dwPlay-ulBufferPos_);
		ulBufferPos_=dwPlay;

		dwRealWritePos=ulBufferPos_+ulBufferLength_;
		dwAvaliableLength=__min(dwWant,dwBufferLength_-ulBufferLength_);
	}
	//有效缓冲区环回了
	else
	{
		dwTmp=ulBufferLength_-(dwBufferLength_-ulBufferPos_);
		if(dwTmp==ulBufferPos_)
		{
			if(dwPlay >= ulBufferPos_ && dwPlay<dwBufferLength_)
			{
				ulBufferLength_-=(dwPlay-ulBufferPos_);
				ulBufferPos_=dwPlay;
			}
			else if(dwPlay >= 0 && dwPlay<dwTmp)
			{
				ulBufferPos_=dwPlay;
				ulBufferLength_=dwTmp-dwPlay;
			}
			//缓冲区是满的，无需加数据
			return 1;
		}

		if(dwPlay >= ulBufferPos_ && dwPlay<dwBufferLength_)
		{
			ulBufferLength_-=(dwPlay-ulBufferPos_);
			ulBufferPos_=dwPlay;
			dwRealWritePos=dwTmp;
			dwAvaliableLength=__min(dwWant,dwBufferLength_-ulBufferLength_);
		}
		else if(dwPlay >=0 && dwPlay<=dwTmp)
		{
			ulBufferLength_=dwTmp-dwPlay;
			ulBufferPos_=dwPlay;
			dwRealWritePos=dwTmp;
			dwAvaliableLength=__min(dwWant,dwBufferLength_-ulBufferLength_);
		}
		else
		{
			TRACE("never here\n");
			dwRealWritePos=dwWrite;
			dwAvaliableLength=__min(dwWant,dwBufferLength_);
			ulBufferPos_=dwRealWritePos;
			ulBufferLength_=dwAvaliableLength;
		}
	}//loop buffer
	return 2;
}//

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

CFastFourierTransform::CFastFourierTransform(int pSampleSize)
{
	xre = NULL;
	xim = NULL;
	mag = NULL;
	fftSin = NULL;
	fftCos = NULL;
	fftBr = NULL;

	ss = pSampleSize;
	ss2 = ss >> 1;
	nu = (int)(log((float)ss) / log((float)2));
	nu1 = nu - 1;

	xre = new float[ss]; // real part
	xim = new float[ss]; // image part
	mag = new float[ss2];

	PrepareFFTTables();
}
CFastFourierTransform::~CFastFourierTransform(void)
{
	if (xre != NULL)
		delete[] xre;

	if (xim != NULL)
		delete[] xim;

	if (mag != NULL)
		delete[] mag;

	if (fftSin != NULL)
		delete[] fftSin;

	if (fftCos != NULL)
		delete[] fftCos;

	if (fftBr != NULL)
		delete[] fftBr;

	xre = NULL;
	xim = NULL;
	mag = NULL;
	fftSin = NULL;
	fftCos = NULL;
	fftBr = NULL;
}
void CFastFourierTransform::PrepareFFTTables()
{
	int n2 = ss2;
	int nu1 = nu - 1;

	fftSin = new float[nu * n2];
	fftCos = new float[nu * n2];

	int k = 0;
	int x = 0;
	for (int l = 1; l <= nu; l++) {
		while (k < ss) {
			for (int i = 1; i <= n2; i++) {
				float p = (float)BitRev(k >> nu1, nu);
				float arg = (PI_2 * p) / (float)ss;
				fftSin[x] = (float)sin(arg);
				fftCos[x] = (float)cos(arg);
				k++;
				x++;
			}

			k += n2;
		}

		k = 0;
		nu1--;
		n2 >>= 1;
	}

	fftBr = new int[ss];
	for (k = 0; k < ss; k++)
		fftBr[k] = BitRev(k, nu);
}
int CFastFourierTransform::BitRev(int j, int nu) {
	int j1 = j;
	int k = 0;
	for (int i = 1; i <= nu; i++) {
		int j2 = j1 >> 1;
		k = ((k << 1) + j1) - (j2 << 1);
		j1 = j2;
	}

	return k;
}
float* CFastFourierTransform::Calculate(float* pSample, size_t pSampleSize) {
	int n2 = ss2;
	int nu1 = nu - 1;
	int wAps = pSampleSize / ss;
	size_t a = 0;

	for (size_t b = 0; a < pSampleSize; b++) {
		xre[b] = pSample[a];
		xim[b] = 0.0F;
		a += wAps;
	}

	int x = 0;
	for (int l = 1; l <= nu; l++) {
		for (int k = 0; k < ss; k += n2) {
			for (int i = 1; i <= n2; i++) {
				float c = fftCos[x];
				float s = fftSin[x];
				int kn2 = k + n2;
				float tr = xre[kn2] * c + xim[kn2] * s;
				float ti = xim[kn2] * c - xre[kn2] * s;
				xre[kn2] = xre[k] - tr;
				xim[kn2] = xim[k] - ti;
				xre[k] += tr;
				xim[k] += ti;
				k++;
				x++;
			}
		}

		nu1--;
		n2 >>= 1;
	}

	for (int k = 0; k < ss; k++) {
		int r = fftBr[k];
		if (r > k) {
			float tr = xre[k];
			float ti = xim[k];
			xre[k] = xre[r];
			xim[k] = xim[r];
			xre[r] = tr;
			xim[r] = ti;
		}
	}

	mag[0] = (float)sqrt(xre[0] * xre[0] + xim[0] * xim[0]) / (float)ss;
	for (int i = 1; i < ss2; i++)
		mag[i] = (2.0F * (float)sqrt(xre[i] * xre[i] + xim[i] * xim[i])) / (float)ss;

	return mag;
}

bool CMp3Show::Init()
{
	EnableCustomNCHitTest(false);
	auto rect=GetRect();

	CHLayout* pBgLayout=new CHLayout(this);
	pBgLayout->SetContentMargin(0, 0, 0, 0);
	CColorLayer* pBgColor=new CColorLayer();
	pBgColor->CreateImageLayerByColor(20, 30, 30);
	pBgColor->SetSizePolicy(SizePolicyExpanding);
	pBgLayout->AddChild(pBgColor);

	return CScene::Init();
}

void CMp3Show::DrawNode()
{
	CNode::DrawNode();

	Gdiplus::Graphics& g= GetView()->GetGraphics();
	Gdiplus::GraphicsPath path;
	Gdiplus::SolidBrush brush(Gdiplus::Color(220, 60, 0));
	Gdiplus::RectF rect;

	auto size = GetSize();
	rect.X = 10.0f;
	rect.Width = (size.first - 10.0f * 2) / 32;

	for (int i = 0; i < 32; ++i)
	{
		rect.X = 10.0f + i*rect.Width;
		rect.Height = size.second*m_fft[i];
		rect.Y = size.second - rect.Height;
		path.AddRectangle(rect);
	}
	g.FillPath(&brush, &path);

	//HDC hMemDC = GetView()->GetMemDC();
	//HBRUSH hBrush = CreateSolidBrush(RGB(220, 60, 0));
	//SelectObject(hMemDC, hBrush);
	//
	//auto size = GetSize();
	//int w = (size.first -10*2)/ 32;
	//RECT rect;
	//rect.left = 10;
	//rect.bottom = (int)size.second;

	//BeginPath(hMemDC);
	//for (int i = 0; i < 32; ++i)
	//{
	//	rect.left = 10 + i*w;
	//	rect.right = rect.left + w;
	//	rect.top = int(rect.bottom - m_pOldFFT[i] * size.second);
	//	Rectangle(hMemDC, rect.left, rect.top, rect.right, rect.bottom);
	//}
	//EndPath(hMemDC);
	//FillPath(hMemDC);
	//DeleteObject(hBrush);
}

bool CMp3PlayerWindow::InitMp3Player()
{
	if (!m_decoder.Initialize("d:\\3.mp3", true))
		return false;

	auto info = m_decoder.SoundInfo();
	m_iAudioLen = info.wf.nAvgBytesPerSec*2;
	m_iAudioLast = 0;
	m_pAudioBuf.reset(new char[m_iAudioLen*2]);

	m_sound.Initialize(info.wf, info.wBitsPerSample, info.wf.nAvgBytesPerSec*2, GetHWND());
	m_sound.Start();
	return WriteAudioData();
}

bool CMp3PlayerWindow::WriteAudioData()
{
	char* pSoundData = m_pAudioBuf.get();
	int len;

	if (m_iAudioLast < m_iAudioLen / 2)
	{
		len = m_decoder.OutputData(pSoundData + m_iAudioLast, m_iAudioLen - m_iAudioLast);
		m_iAudioLast += len;
	}
	if (m_iAudioLast < 1)
	{
		TRACE("end of mp3\n");
		m_sound.Stop();
		return false;
	}

	DWORD written, writePos;
	int res=m_sound.Write(pSoundData, m_iAudioLast, written, writePos);
	if(res == 2)
	{
		{
			char *p=pSoundData + m_iAudioLen;
			if(writePos+written < (DWORD)m_iAudioLen)
			{
				memcpy(p+writePos, pSoundData, written);
			}
			else
			{
				len=m_iAudioLen - writePos;
				memcpy(p+writePos, pSoundData, len);
				memcpy(p, pSoundData+len, written-len);
			}
		}
		memcpy(pSoundData, pSoundData+written, m_iAudioLast-written);
		m_iAudioLast -=written;
	}

	return res != 0;
}

void CMp3PlayerWindow::GetSpectrum()
{
	int pos;
	if(!m_sound.SamplePosition(pos))
		return;

	auto pAudio = m_pAudioBuf.get() + m_iAudioLen;
	auto& soundInfo = m_sound.SoundFormat();
	auto* pSampleFloat = m_pSamplesFloat.get();
	float left, right;
	int len, i;
	char temp[4];

	if (soundInfo.nChannels == 2 && soundInfo.wBitsPerSample == 16)
	{
		len = m_iSampleSize * 2 * 2;
		if (pos + len < m_iAudioLen)
		{
			memcpy(m_pSamples.get(), pAudio + pos, len);
		}
		else
		{
			i = m_iAudioLen - pos;
			memcpy(m_pSamples.get(), pAudio + pos, i);
			memcpy(m_pSamples.get() + i, pAudio, len - i);
		}

		for (i = 0;i < m_iSampleSize;++i)
		{
			memcpy(temp, m_pSamples.get() + i * 4, 4);
			left = ((temp[1] << 8) + temp[0]) / 32767.0f;
			right = ((temp[3] << 8) + temp[2]) / 32767.0f;
			*(pSampleFloat + i) = (left + right) / 2;
		}
	}
	else
	{
		throw 1;
		return;
	}

	auto pRes=m_fft.Calculate(m_pSamplesFloat.get(), m_iSampleSize);
	float wFs;
	len = m_iFFTSize / 2 / 32;
	for (i = 0;i<32;++i)
	{
		wFs = 0.0f;
		for (int j = 0;j< len;++j)
		{
			wFs += pRes[i*len + j];
		}
		wFs = (wFs * (float)log(i + 2.0F));

		if (wFs > 0.005F && wFs < 0.009F)
			wFs *= 50.0F;
		else if (wFs > 0.01F && wFs < 0.1F)
			wFs *= 10.0F;
		else if (wFs > 0.1F && wFs < 0.5F)
			wFs *= PI; //enlarge PI times, if do not, the bar display abnormally, why??

		if (wFs > 1.0F)
		{
			wFs = 1.0F;
		}

		if (wFs >= m_pOldFFT[i])
		{
			m_pOldFFT[i] = wFs;
		}
		else
		{
			m_pOldFFT[i] -= 0.015f;
		}
	}
}

CMp3PlayerWindow::CMp3PlayerWindow():
	m_iSampleSize(512),
	m_iFFTSize(512),
	m_fft(m_iFFTSize),
	m_pShow(NULL)
{
	// left and right
	m_pSamples.reset(new char[m_iSampleSize * 2 * 2]);
	m_pSamplesFloat.reset(new float[m_iSampleSize]);
	m_pOldFFT.reset(new float[32]);
}

LRESULT CMp3PlayerWindow::CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed)
{
	if(message == WM_CREATE)
	{
		ReSize(800, 600, true);
		SetWindowText(GetHWND(), TEXT("music"));

		CGDIView* pView=new CGDIView();
		pView->Init(GetHWND());
		m_pDir.reset(new CDirector(pView));
		m_pShow=new CMp3Show();
		assert(m_pShow);
		m_pDir->RunScene(m_pShow);

		if(InitMp3Player())
		{
			SetTimer(hWnd, 101, 400, NULL);
		}
	}
	else if(message == WM_TIMER)
	{
		if(!WriteAudioData())
		{
			KillTimer(hWnd, 101);
		}
		return CBaseWindow::CustomProc(hWnd, message, wParam, lParam, bProcessed);
	}

	if(m_pDir.get())
	{
		auto res=m_pDir->MessageProc(message, wParam, lParam, bProcessed);
		if(bProcessed)
		{
			return res;
		}
	}

	return CBaseWindow::CustomProc(hWnd, message, wParam, lParam, bProcessed);
}

int CMp3PlayerWindow::Run()
{
	LARGE_INTEGER fre;
	LARGE_INTEGER last, now;
	LARGE_INTEGER gap;

	QueryPerformanceFrequency(&fre);
	QueryPerformanceCounter(&last);
	gap.QuadPart = LONGLONG(20.0 / 1000 * fre.QuadPart);

	MSG msg;
	while(true)
	{
		if(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			QueryPerformanceCounter(&now);
			if(now.QuadPart - last.QuadPart > gap.QuadPart)
			{
				GetSpectrum();
				memcpy(&m_pShow->m_fft[0], m_pOldFFT.get(), sizeof(m_pShow->m_fft));
				m_pShow->DrawScene();
				last.QuadPart=now.QuadPart;
			}
			else
				Sleep(1);
		}

		if(msg.message == WM_QUIT)
			break;
		if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}