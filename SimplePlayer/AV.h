#pragma once

extern "C"
{
#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libavutil\avutil.h>
#include <libavutil\time.h>
#include <libavutil\imgutils.h>
#include <libswscale\swscale.h>
#include <libswresample\swresample.h>
}

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")


#include <memory>
#include <string>
#include <queue>
#include <Node/RingBuffer.h>
#include "AVIOContext.h"

struct AudioParams
{
	int sample_rate;
	int channels;
	int64_t channel_layout;
	enum AVSampleFormat sample_fmt;

	AudioParams(int fre, int channels, int64_t layout, AVSampleFormat fmt)
		:sample_rate(fre),
		channels(channels),
		channel_layout(layout),
		sample_fmt(fmt)
	{
	}
	AudioParams()
		:sample_rate(44100),
		channels(2),
		channel_layout(av_get_default_channel_layout(2)),
		sample_fmt(AV_SAMPLE_FMT_S16)
	{
	}
};

struct VideoParams
{
	int width;
	int heigh;
	enum AVPixelFormat fmt;

	VideoParams(int w, int h, AVPixelFormat fmt)
		:width(w),
		heigh(h),
		fmt(fmt)
	{
	}
	VideoParams()
		:width(0),
		heigh(0),
		fmt(AV_PIX_FMT_RGB24)
	{
	}
};

struct FrameInfo
{
	double pts;
	int width;
	int height;
	int dataSize;
};

class CSimpleDecoder
{
public:
	CSimpleDecoder();
	virtual ~CSimpleDecoder();
	int Interrupt();

	bool HasVideo() { return m_iVideoIndex != -1; }
	bool HasAudio() { return m_iAudioIndex != -1; }
	bool LoadFile(std::string fileName);
	void Clean();
	void SetCustomIOContext(IIOContext* pIO);
	virtual bool SeekTime(double target_pos, double currPos);
	double AudioBaseTime() { return m_dCurrentAudioPts; }

	int ReadPacket(AVPacket* pPacket);
	bool ConfigureAudioOut(AudioParams* destAudioParams = NULL, AudioParams* srcAudioParams = NULL);
	bool ConfigureVideoOut(VideoParams* destVideoParams = NULL, VideoParams* srcVideoParams = NULL);
	bool DecodeAudio(RingBuffer*pBuf, int& got);
	bool DecodeAudio(uint8_t *rcv_buf, int buf_want_len, int& got_len);
	bool DecodeVideo(uint8_t *rcv_buf, int buf_len, FrameInfo& info);
	bool DecodeVideo(RingBuffer*pBuf, FrameInfo& info);

	int64_t GetDurationAll();
	double GetFrameRate();
	int GetSampleRate();
	std::pair<int, int>GetFrameSize();
	bool EndOfFile() { return m_bEndOf && m_VideoPacket.empty() && m_AudioPacket.empty(); }
protected:
	void ReverseCurrentImage();
protected:
	int m_iInterrupt;
	IIOContext* m_pCustomIOContext;
	AVIOContext* m_pIOContext;
	AVFormatContext* m_pFormatContext;
	int m_iVideoIndex;
	int m_iAudioIndex;
	AVCodecContext* m_pVCodecContext;
	AVCodecContext* m_pACodecContext;
	AVCodec* m_pVCodec;
	AVCodec* m_pACodec;
	bool m_bEndOf;
	AVFrame* m_pAFrame;
	AVFrame* m_pVFrame;

	AudioParams m_outAudioParams;
	AudioParams m_srcAudioParams;
	SwrContext* m_pASwr;
	uint8_t* m_pAudioOutBuf;
	int m_iAudioOutLen;
	int m_iAudioOutRemaind;
	double m_dCurrentAudioPts;

	VideoParams m_outVideoParams;
	VideoParams m_srcVideoParams;
	SwsContext* m_pVSws;
	uint8_t* m_aVideoOutBuf[4];
	int    m_aVideoOutLines[4];
	bool m_bCurrentImageNotCopy;
	uint8_t* m_pLineForReverse;
	double m_dCurrentImagePts;
	double m_dVideotb;

	std::queue<AVPacket> m_AudioPacket;
	std::queue<AVPacket> m_VideoPacket;
};

#include <Node/Thread.h>
class CDecodeLoop : public CSimpleDecoder, public CMessageLoop
{
public:
	int Run();
	bool Init();
	void Destroy();
	bool SeekTime(double target_pos, double currPos);

	int GetAudioData(RingBuffer* pBuf, int want);
	int GetImageData(RingBuffer* pBuf, FrameInfo& info);
protected:
	inline void CacheAudioData();
	inline void CacheImageData();
protected:
	std::unique_ptr<RingBuffer> m_pSoundBuf;
	std::unique_ptr<RingBuffer> m_pImageBuf;
	int m_iCachedImageCount;
	CSimpleLock m_audioLock;
	CSimpleLock m_videoLock;
	FrameInfo m_frameDump;
};