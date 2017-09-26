#pragma once

#include "AV.h"

class AVDecodeMulti : public CDecodeLoop
{
public:
	AVDecodeMulti();
	~AVDecodeMulti();

	bool Init();
	void Destroy();
	bool SeekTime(double target_pos, double currPos);
protected:
	void ReadThread();
	void AudioThread();
	void VideoThread();

protected:
	double m_dSeekTargetPos;
	double m_dSeekCurrPos;
	int m_iSeekRequest;
	int m_iFlushVideoBufRequest;
	int m_iFlushAudioBufRequest;
	std::thread m_threadRead;
	std::thread m_threadAudio;
	std::thread m_threadVideo;

	std::mutex m_mutexRead;
	std::condition_variable m_cvRead;
};
