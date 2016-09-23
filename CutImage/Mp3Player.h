#pragma once
#include "BaseWindow.h"
#include "Node.h"
#include <dsound.h>
#include <mmsystem.h>
#include <wmsdk.h>

class CSound
{
public:
	CSound();
	~CSound();

	bool Initialize(WAVEFORMAT wf, WORD wBitsPerSample, DWORD dwBufferLength, HWND hWnd);
	bool Clear();
	int Start();
	void Stop();
	int Write(void *pData, DWORD dwLen, DWORD &dwWritten, DWORD& dwWritePos);
	DWORD BufferLength()const;
	WAVEFORMATEX SoundFormat()const;
	void Seek();
	void SamplePosition(int &samplePos, int &bufferStart, int bufferLen);
	void SamplePosition(int &samplePos);
	//�����δ�����ŵ����ݵ�ʱ��(����)
	unsigned int GetRemainderTime();
protected:
	//type=0�����������������
	void ClearBuffer(int type);
	//��û������пɱ�д��������(���Ḳ����δ��������)����ʼλ�ã��Լ���д�ĳ���
	int AvaliableBuffer(DWORD dwWant, DWORD &dwRealWritePos, DWORD &dwAvaliableLength);
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

class CMp3Show : public CScene
{
public:
	CMp3Show();
	bool Init();
	LRESULT MessageProc(UINT, WPARAM, LPARAM, bool& bProcessed);
protected:
	void InitMp3Player();
	void WriteAudioData();
protected:
	std::unique_ptr<char[]> m_pAudioBuf;
	int m_iAudioLen;
	int m_iAudioLast;
	CSound m_sound;
	CWM m_decoder;
	HANDLE m_hTimerQueue;
	HANDLE m_hTimer;
};

class CMp3PlayerWindow : public CBaseWindow
{
public:
	LRESULT CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed);
protected:
	std::unique_ptr<CDirector> m_pDir;
};