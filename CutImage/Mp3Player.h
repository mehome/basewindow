#pragma once
#include "BaseWindow.h"
#include "Node.h"
#include "Thread.h"
#include <dsound.h>
#include <mmsystem.h>
#include <wmsdk.h>

class CSound
{
public:
	CSound();
	~CSound();

	bool Initialize(WAVEFORMAT wf,WORD wBitsPerSample,DWORD dwBufferLength,HWND hWnd);
	bool Clear();
	int Start();
	void Stop();
	bool IsPlaying()const { return bPlaying_; }
	int Write(void *pData,DWORD dwLen,DWORD &dwWriteLen, DWORD& dwWritePos);
	const WAVEFORMATEX& SoundFormat()const;
	void Seek();
	bool SamplePosition(int &samplePos);
protected:
	//type=0�����������������
	void ClearBuffer(int type);
	//��û������пɱ�д��������(���Ḳ����δ��������)����ʼλ�ã��Լ���д�ĳ���
	int AvaliableBuffer(DWORD dwWant,DWORD &dwRealWritePos,DWORD &dwAvaliableLength);
protected:
	bool bPlaying_;
	LPDIRECTSOUND lpDS_;
	LPDIRECTSOUNDBUFFER lpDSB_;
	WAVEFORMATEX waveFormat_;
	DWORD dwBufferLength_;
	DWORD dwSilenceBytes_;

	//��¼�ϴ�д��������λ��
	int iWritePos_;
	//��¼��Ч����������ʼλ��
	unsigned long ulBufferPos_;
	//��¼��Ч�������ĳ���
	unsigned long ulBufferLength_;
	//д������ʱ������λ��
	DWORD dwLockOffset_;
	//д������ʱ�����ĳ���
	DWORD dwLockLen_;
};

class CWM
{
public:
	CWM();
	~CWM();

	void Clear();
	bool Initialize(std::string zFileName, bool bDiscrete = false);
	bool SetFileName(std::string szName);
	int OutputData(char *pDest, int iWantLen);
	PCMWAVEFORMAT SoundInfo();
	void Seek(unsigned int uiMS);
	unsigned int GetSoundLength();
	unsigned int GetCurrentPos();
protected:
	IWMSyncReader *pWMSyncReader_;
	IWMHeaderInfo *pHeaderInfo_;
	INSSBuffer *pNSSBuffer_;

	PCMWAVEFORMAT pcmWF_;
	bool bHasAudio_;
	unsigned int dwOutput_;
	WORD wStream_;
	LARGE_INTEGER liDuration_;
	LARGE_INTEGER liReadedDuration_;

	unsigned int dwLastRead_;
};

class CFastFourierTransform
{
private:
	float* xre;
	float* xim;
	float* mag;
	float* fftSin;
	float* fftCos;
	int* fftBr;
	int ss;
	int ss2;
	int nu;
	int nu1;

	int BitRev(int j, int nu);
	void PrepareFFTTables();
public:
	CFastFourierTransform(int pSampleSize);
	~CFastFourierTransform(void);

	float* Calculate(float* pSample, size_t pSampleSize);
};

class CMp3Show : public CScene
{
public:
	bool Init();
	void DrawNode();
	float m_fft[32];
};

class CMp3PlayerWindow : public CBaseWindow , public CApplication
{
public:
	CMp3PlayerWindow();
	LRESULT CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed);
	int Run();
protected:
	bool InitMp3Player();
	bool WriteAudioData();
	void GetSpectrum();
protected:
	std::unique_ptr<CDirector> m_pDir;
	const int m_iSampleSize;
	const int m_iFFTSize;
	std::unique_ptr<char[]> m_pAudioBuf;
	std::unique_ptr<char[]> m_pSamples;
	std::unique_ptr<float[]> m_pSamplesFloat;
	std::unique_ptr<float[]> m_pOldFFT;
	int m_iAudioLen;
	int m_iAudioLast;
	CSound m_sound;
	CWM m_decoder;
	CFastFourierTransform m_fft;

	CMp3Show* m_pShow;
};