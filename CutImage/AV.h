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


#include <string>
#include <queue>

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

class CSimpleDecoder
{
public:
	CSimpleDecoder();
	int Interrupt();

	bool LoadFile(std::string fileName);
	void Clean();

	int ReadPacket(AVPacket* pPacket);
	bool ConfigureAudioOut(AudioParams* srcAudioParams = NULL);
	bool ConfigureVideoOut(VideoParams* destVideoParams = NULL, VideoParams* srcVideoParams = NULL);
	bool DecodeAudio(uint8_t *rcv_buf, int buf_want_len, int& got_len);
	bool DecodeVideo(uint8_t *rcv_buf, int buf_len);
	void test();

	int64_t GetDurationAll();
	double GetFrameRate();
protected:
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

	VideoParams m_outVideoParams;
	VideoParams m_srcVideoParams;
	SwsContext* m_pVSws;
	uint8_t* m_aVideoOutBuf[4];
	int    m_aVideoOutLines[4];

	std::queue<AVPacket> m_AudioPacket;
	std::queue<AVPacket> m_VideoPacket;
};