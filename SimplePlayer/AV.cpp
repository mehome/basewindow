
#include <cassert>
#include <iostream>
#include <windows.h>
#include "AV.h"
#include <Node\Log.h>

int InterruptCB(void* para)
{
	CSimpleDecoder* pDecoder = (CSimpleDecoder*)para;
	return pDecoder->Interrupt();
}

CSimpleDecoder::CSimpleDecoder():
	m_iInterrupt(0),
	m_pCustomIOContext(NULL),
	m_pIOContext(NULL),
	m_pFormatContext(NULL),
	m_iAudioIndex(-1),
	m_iVideoIndex(-1),
	m_pACodecContext(NULL),
	m_pVCodecContext(NULL),
	m_pVCodec(NULL),
	m_pACodec(NULL),
	m_bEndOf(true),
	m_pASwr(NULL),
	m_pAudioOutBuf(NULL),
	m_iAudioOutLen(0),
	m_iAudioOutRemaind(0),
	m_dCurrentAudioPts(-0.0001),
	m_pVSws(NULL),
	m_bCurrentImageNotCopy(false),
	m_pLineForReverse(NULL)
{
	m_pAFrame = av_frame_alloc();
	m_pVFrame = av_frame_alloc();
	memset(m_aVideoOutBuf, 0, sizeof(m_aVideoOutBuf));
}

CSimpleDecoder::~CSimpleDecoder()
{
	Clean();
	av_frame_free(&m_pAFrame);
	av_frame_free(&m_pVFrame);
}

int CSimpleDecoder::Interrupt()
{
	return m_iInterrupt;
}

bool CSimpleDecoder::LoadFile(std::string fileName)
{
	unsigned int n;
	AVStream* pStream;

	if (m_iAudioIndex != -1 || m_iVideoIndex != -1)
	{
		return false;
	}

	m_pFormatContext = avformat_alloc_context();
	m_pFormatContext->interrupt_callback.callback = InterruptCB;
	m_pFormatContext->interrupt_callback.opaque = this;

	if (m_pCustomIOContext)
	{
		assert(m_pCustomIOContext->Buf());
		m_pIOContext = avio_alloc_context(m_pCustomIOContext->Buf(), m_pCustomIOContext->BufLen(), 0, m_pCustomIOContext, ReadIO, WriteIO, SeekIO);
		m_pFormatContext->pb = m_pIOContext;
		AVSEEK_SIZE;
	}

	n = avformat_open_input(&m_pFormatContext, fileName.c_str(), NULL, NULL);
	if (n != 0)
	{
		return false;
	}
	n = avformat_find_stream_info(m_pFormatContext, NULL);
	if (n < 0)
	{
		avformat_close_input(&m_pFormatContext);
		return false;
	}

	//for (n = 0; n < m_pFormatContext->nb_streams; ++n)
	//{
	//	pStream = m_pFormatContext->streams[n];
	//	switch (pStream->codecpar->codec_type)
	//	{
	//	case AVMEDIA_TYPE_VIDEO:
	//		m_iVideoIndex = n;
	//		break;
	//	case AVMEDIA_TYPE_AUDIO:
	//		if(m_iAudioIndex == -1)m_iAudioIndex = n;
	//		break;
	//	}
	//}
	m_iAudioIndex = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	m_iVideoIndex = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

	if (m_iAudioIndex == AVERROR_DECODER_NOT_FOUND && m_iVideoIndex == AVERROR_DECODER_NOT_FOUND)
	{
		return false;
	}

	int lowres;
	if (m_iVideoIndex != -1)
	{
		m_pVCodecContext = avcodec_alloc_context3(NULL);
		avcodec_parameters_to_context(m_pVCodecContext, m_pFormatContext->streams[m_iVideoIndex]->codecpar);
		av_codec_set_pkt_timebase(m_pVCodecContext, m_pFormatContext->streams[m_iVideoIndex]->time_base);
		m_pVCodec = avcodec_find_decoder(m_pVCodecContext->codec_id);
		lowres = 0;
		if (lowres > av_codec_get_max_lowres(m_pVCodec))
			lowres = av_codec_get_max_lowres(m_pVCodec);
		av_codec_set_lowres(m_pVCodecContext, lowres);
		avcodec_open2(m_pVCodecContext, m_pVCodec, NULL);
		m_dVideotb = av_q2d(m_pFormatContext->streams[m_iVideoIndex]->time_base);
	}
	if (m_iAudioIndex != -1)
	{
		m_pACodecContext = avcodec_alloc_context3(NULL);
		avcodec_parameters_to_context(m_pACodecContext, m_pFormatContext->streams[m_iAudioIndex]->codecpar);
		av_codec_set_pkt_timebase(m_pACodecContext, m_pFormatContext->streams[m_iAudioIndex]->time_base);
		m_pACodec = avcodec_find_decoder(m_pACodecContext->codec_id);
		lowres = 0;
		if (lowres > av_codec_get_max_lowres(m_pACodec))
			lowres = av_codec_get_max_lowres(m_pACodec);
		av_codec_set_lowres(m_pACodecContext, lowres);
		avcodec_open2(m_pACodecContext, m_pACodec, NULL);
	}
	m_bEndOf = false;
	return true;
}

void CSimpleDecoder::Clean()
{
	while (!m_VideoPacket.empty())
	{
		av_packet_unref(&m_VideoPacket.front());
		m_VideoPacket.pop_front();
	}
	while (!m_AudioPacket.empty())
	{
		av_packet_unref(&m_AudioPacket.front());
		m_AudioPacket.pop_front();
	}

	if (m_aVideoOutBuf[0])
	{
		av_freep(&m_aVideoOutBuf[0]);
	}
	if (m_pVSws)
	{
		sws_freeContext(m_pVSws);
		m_pVSws = NULL;
	}
	if (m_pLineForReverse)
	{
		av_freep(&m_pLineForReverse);
	}

	if (m_pAudioOutBuf)
	{
		av_freep(&m_pAudioOutBuf);
	}
	if (m_pASwr)
	{
		swr_free(&m_pASwr);
	}

	if (m_pACodecContext)
	{
		avcodec_free_context(&m_pACodecContext);
	}
	if (m_pVCodecContext)
	{
		avcodec_free_context(&m_pVCodecContext);
	}
	if (m_pIOContext)
	{
		av_freep(&m_pIOContext->buffer);
		av_freep(&m_pIOContext);
	}
	if (m_pCustomIOContext)
	{
		m_pCustomIOContext->Reset();
		delete m_pCustomIOContext;
		m_pCustomIOContext = NULL;
	}
	if (m_pFormatContext)
		avformat_close_input(&m_pFormatContext);
	m_bEndOf = false;
	m_iVideoIndex = -1;
	m_iAudioIndex = -1;
}

void CSimpleDecoder::SetCustomIOContext(IIOContext* pIO)
{
	if (m_iAudioIndex != -1 || m_iVideoIndex != -1)
	{
		return;
	}
	if (m_pCustomIOContext)
	{
		delete m_pCustomIOContext;
		m_pCustomIOContext = NULL;
	}
	m_pCustomIOContext = pIO;
}

bool CSimpleDecoder::SeekTime(double target_pos, double currPos)
{
	while (!m_VideoPacket.empty())
	{
		av_packet_unref(&m_VideoPacket.front());
		m_VideoPacket.pop_front();
	}
	while (!m_AudioPacket.empty())
	{
		av_packet_unref(&m_AudioPacket.front());
		m_AudioPacket.pop_front();
	}
	m_bCurrentImageNotCopy = false;
	m_iAudioOutRemaind = 0;
	m_dCurrentAudioPts = -0.0001;

	int64_t minpts, maxpts, ts;

	ts = (int64_t)(target_pos*AV_TIME_BASE);
	if (currPos >= target_pos)
	{
		minpts = INT64_MIN;
		maxpts = (int64_t)(currPos*AV_TIME_BASE - 2);
	}
	else
	{
		minpts = (int64_t)(currPos*AV_TIME_BASE + 2);
		maxpts = INT64_MAX;
	}
	auto res = avformat_seek_file(m_pFormatContext, -1, minpts, ts, maxpts, 0);
	if (0 <= res)
	{
		avcodec_flush_buffers(m_pVCodecContext);
		avcodec_flush_buffers(m_pACodecContext);
		return true;
	}
	return false;
}

int CSimpleDecoder::ReadPacket(AVPacket* pPacket)
{
	int res = av_read_frame(m_pFormatContext, pPacket);
	if (res < 0)
	{
		if (res == AVERROR_EOF || avio_feof(m_pFormatContext->pb))
		{
			// 文件结尾
			m_bEndOf = true;
		}
		m_bEndOf = true;
	}

	return res;
}

bool CSimpleDecoder::ConfigureAudioOut(AudioParams* destAudioParams, AudioParams* pSrcAudioParams)
{
	if (m_iAudioIndex == -1)
		return false;

	if (!destAudioParams)
	{
		m_outAudioParams = AudioParams();
	}
	else
	{
		m_outAudioParams = *destAudioParams;
	}

	if(!pSrcAudioParams)
	{
		m_srcAudioParams.sample_rate = m_pACodecContext->sample_rate;
		m_srcAudioParams.sample_fmt = m_pACodecContext->sample_fmt;
		m_srcAudioParams.channels = m_pACodecContext->channels;
		m_srcAudioParams.channel_layout = m_pACodecContext->channel_layout;

		if (!m_srcAudioParams.channel_layout ||
			av_get_channel_layout_nb_channels(m_srcAudioParams.channel_layout) != m_srcAudioParams.channels)
		{
			return false;
		}
	}
	else
	{
		m_srcAudioParams = *pSrcAudioParams;
	}

	// 音频输出的格式是固定的
	m_pASwr = swr_alloc_set_opts(m_pASwr,
		m_outAudioParams.channel_layout,
		m_outAudioParams.sample_fmt,
		m_outAudioParams.sample_rate,
		m_srcAudioParams.channel_layout,
		m_srcAudioParams.sample_fmt,
		m_srcAudioParams.sample_rate, 0, NULL);

	if (!m_pASwr)
	{
		return false;
	}
	if (swr_init(m_pASwr) < 0)
	{
		swr_free(&m_pASwr);
		return false;
	}

	if (!m_pAudioOutBuf)
	{
		m_iAudioOutRemaind = 0;
		m_iAudioOutLen = 0;
		av_fast_malloc(&this->m_pAudioOutBuf, (unsigned int*)&m_iAudioOutLen,
			av_samples_get_buffer_size(NULL, m_outAudioParams.channels, 2048, m_outAudioParams.sample_fmt, 0));
		assert(m_pAudioOutBuf != NULL);
	}

	return true;
}

bool CSimpleDecoder::ConfigureVideoOut(VideoParams* destVideoParams, VideoParams* srcVideoParams)
{
	if (m_iVideoIndex == -1)
		return false;

	if (destVideoParams == NULL)
	{
		m_outVideoParams.width = m_pVCodecContext->width;
		m_outVideoParams.heigh = m_pVCodecContext->height;
		m_outVideoParams.fmt = AV_PIX_FMT_BGR24;
	}
	else
	{
		m_outVideoParams = *destVideoParams;
	}

	if (srcVideoParams == NULL)
	{
		m_srcVideoParams.width = m_pVCodecContext->width;
		m_srcVideoParams.heigh = m_pVCodecContext->height;
		m_srcVideoParams.fmt = m_pVCodecContext->pix_fmt;
	}
	else
	{
		m_srcVideoParams = *srcVideoParams;
	}

	m_pVSws = sws_getCachedContext(m_pVSws,
		m_srcVideoParams.width,
		m_srcVideoParams.heigh,
		m_srcVideoParams.fmt,
		m_outVideoParams.width,
		m_outVideoParams.heigh,
		m_outVideoParams.fmt,
		SWS_BICUBIC, NULL, NULL, NULL);
	if (!m_pVSws)
		return false;

	if (m_aVideoOutBuf[0])
	{
		av_freep(&m_aVideoOutBuf[0]);
	}
	if (0 > av_image_alloc(m_aVideoOutBuf, m_aVideoOutLines, m_outVideoParams.width, m_outVideoParams.heigh, m_outVideoParams.fmt, 4))
	{
		return false;
	}

	//if (m_pLineForReverse)
	//{
	//	av_freep(&m_pLineForReverse);
	//}
	//if (m_outVideoParams.fmt == AV_PIX_FMT_BGR24)
	//{
	//	m_pLineForReverse = (uint8_t*)av_malloc(m_aVideoOutLines[0]);
	//}
	return true;
}

bool CSimpleDecoder::DecodeAudio(RingBuffer* pBuf, int& len)
{
	int res, n, outCount, outSize;
	int64_t thisFrameChannelLayout;
	AVPacket packet;
	bool bWaitOtherSetp(false);
	int want = pBuf->WriteableBufferLen();
	len = 0;
	n = min(want, m_iAudioOutRemaind);
	if (n > 0)
	{
		pBuf->WriteData((char*)m_pAudioOutBuf, n);
		len = n;
		want -= n;

		if (m_iAudioOutRemaind > n)
		{
			memcpy(m_pAudioOutBuf, m_pAudioOutBuf + n, m_iAudioOutRemaind - n);
		}
		m_iAudioOutRemaind -= n;
	}
	while (want > 0)
	{
		// step 1,读取数据
		while (1)
		{
			if (m_AudioPacket.empty())
			{
				res = ReadPacket(&packet);
				if (res != 0)
					return false;
			}
			else
			{
				packet = m_AudioPacket.front();
				m_AudioPacket.pop_front();
			}

			if (packet.stream_index == m_iAudioIndex)
			{
				//avcodec_decode_audio4(m_pACodecContext, m_pAFrame, &got, &packet);
				res = avcodec_send_packet(m_pACodecContext, &packet);
				if (res == 0)
				{
					av_packet_unref(&packet);
					bWaitOtherSetp = false;
					break;
				}
				else
				{
					if (res == AVERROR(EAGAIN))
					{
						m_AudioPacket.push_front(packet);
						if (bWaitOtherSetp)
						{
							return false;
						}
						bWaitOtherSetp = true;
						break;
					}
					else
					{
						av_packet_unref(&packet);
						return false;
					}
				}
			}
			else if (packet.stream_index == m_iVideoIndex)
			{
				m_VideoPacket.push_back(packet);
			}
			else
			{
				av_packet_unref(&packet);
			}
		}

		// step 2,解码部分
		res = avcodec_receive_frame(m_pACodecContext, m_pAFrame);
		if (res == 0)
		{
			if (m_dCurrentAudioPts < 0 && m_pAFrame->pts != AV_NOPTS_VALUE)
			{
				m_dCurrentAudioPts = m_pAFrame->pts * av_q2d(av_codec_get_pkt_timebase(m_pACodecContext));
			}

			bWaitOtherSetp = false;
			thisFrameChannelLayout =
				(m_pAFrame->channel_layout && av_frame_get_channels(m_pAFrame) == av_get_channel_layout_nb_channels(m_pAFrame->channel_layout)) ?
				m_pAFrame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(m_pAFrame));

			if (thisFrameChannelLayout != m_srcAudioParams.channel_layout ||
				m_pAFrame->sample_rate != m_srcAudioParams.sample_rate ||
				m_pAFrame->format != m_srcAudioParams.sample_fmt || 
				!m_pASwr)
			{
				if (!ConfigureAudioOut(&m_outAudioParams, &AudioParams(m_pAFrame->sample_rate, av_frame_get_channels(m_pAFrame), thisFrameChannelLayout, (AVSampleFormat)m_pAFrame->format)))
				{
					return false;
				}
			}

			outCount = swr_get_out_samples(m_pASwr, m_pAFrame->nb_samples);
			if (outCount < 0)
			{
				return false;
			}
			outSize = av_samples_get_buffer_size(NULL, m_outAudioParams.channels, outCount, m_outAudioParams.sample_fmt, 0);
			if (outSize > m_iAudioOutLen)
			{
				av_fast_malloc(&m_pAudioOutBuf, (unsigned int*)&m_iAudioOutLen, outSize);
				if (!m_pAudioOutBuf)
					return false;
			}

			res = swr_convert(this->m_pASwr, &m_pAudioOutBuf, outCount, (const uint8_t**)m_pAFrame->extended_data, m_pAFrame->nb_samples);
			if (res < 0)
				return false;
			outSize = av_samples_get_buffer_size(NULL, m_outAudioParams.channels, res, m_outAudioParams.sample_fmt, 1);
			n = min(outSize, want);
			pBuf->WriteData((char*)m_pAudioOutBuf, n);
			want -= n;
			len += n;
			if (n < outSize)
			{
				m_iAudioOutRemaind = outSize - n;
				memcpy(m_pAudioOutBuf, m_pAudioOutBuf + n, m_iAudioOutRemaind);
			}
			else
				m_iAudioOutRemaind = 0;
		}
		else if (res == AVERROR(EAGAIN))
		{
			if (bWaitOtherSetp)
			{
				return false;
			}
			bWaitOtherSetp = true;
		}
		else
		{
			// 其他错误
			return false;
		}
	}
	return true;
}

bool CSimpleDecoder::DecodeAudio(uint8_t *rcv_buf, int buf_want_len, int& got_len)
{
	assert(rcv_buf && buf_want_len > 0);
	RingBuffer rb(buf_want_len, (char*)rcv_buf);
	return DecodeAudio(&rb, got_len);
}

bool CSimpleDecoder::DecodeVideo(RingBuffer*pBuf, FrameInfo& info)
{
	AVPacket packet;
	int res;
	uint64_t pts;

	info.dataSize = 0;
	if (m_bCurrentImageNotCopy)
	{
		res = m_outVideoParams.heigh;
		goto Copy;
	}
ReadPacket:
	if (m_VideoPacket.empty())
	{
		res = ReadPacket(&packet);
		if (res != 0)
			return false;
	}
	else
	{
		packet = m_VideoPacket.front();
		m_VideoPacket.pop_front();
	}
	if (packet.stream_index == m_iVideoIndex)
	{
		res = avcodec_send_packet(m_pVCodecContext, &packet);
		if (res == 0)
		{
			av_packet_unref(&packet);
		}
		else
		{
			if (res == AVERROR(EAGAIN))
			{
				m_VideoPacket.push_front(packet);
			}
			else if(res == AVERROR_INVALIDDATA)
			{
				av_packet_unref(&packet);
			}
			else
			{
				// AVERROR(EOF) etc
				return false;
			}
		}
	}
	else if(packet.stream_index == m_iAudioIndex)
	{
		m_AudioPacket.push_back(packet);
	}
	else
	{
		av_packet_unref(&packet);
	}

	res = avcodec_receive_frame(m_pVCodecContext, m_pVFrame);
	if (0 == res)
	{
		pts = av_frame_get_best_effort_timestamp(m_pVFrame);
		m_dCurrentImagePts = pts == AV_NOPTS_VALUE ? NAN : pts*m_dVideotb;
		if (m_pVFrame->format != m_srcVideoParams.fmt ||
			m_pVFrame->width != m_srcVideoParams.width ||
			m_pVFrame->height != m_srcVideoParams.heigh ||
			!m_pVSws)
		{
			if (!ConfigureVideoOut(&m_outVideoParams, &VideoParams(m_pVFrame->width, m_pVFrame->height, (AVPixelFormat)m_pVFrame->format)))
				return false;
		}

		res = sws_scale(m_pVSws,m_pVFrame->data, m_pVFrame->linesize, 0, m_pVFrame->height, m_aVideoOutBuf, m_aVideoOutLines);
		assert(res == m_outVideoParams.heigh);
		//if (m_pLineForReverse)
		//{
		//	ReverseCurrentImage();
		//}
	Copy:
		info.dataSize = av_image_get_buffer_size(m_outVideoParams.fmt, m_outVideoParams.width, res, 4);
		info.width = m_outVideoParams.width;
		info.height = res;
		info.pts = m_dCurrentImagePts;
		if (pBuf && info.dataSize <= pBuf->WriteableBufferLen())
		{
			pBuf->WriteData((char*)m_aVideoOutBuf[0], info.dataSize);
			m_bCurrentImageNotCopy = false;
		}
		else
		{
			m_bCurrentImageNotCopy = true;
			return pBuf == NULL;
		}
	}
	else if (res == AVERROR(EAGAIN))
	{
		goto ReadPacket;
	}
	else
		return false;

	return true;
}

bool CSimpleDecoder::DecodeVideo(uint8_t *buf, int buf_len, FrameInfo& info)
{
	assert(buf && buf_len > 0);
	RingBuffer rb(buf_len, (char*)buf);
	return DecodeVideo(&rb, info);
}

int64_t CSimpleDecoder::GetDurationAll()
{
	int64_t all = m_pFormatContext->duration;
	return all / AV_TIME_BASE;
}

double CSimpleDecoder::GetFrameRate()
{
	if(m_iVideoIndex != -1)
	{
		auto pStream = m_pFormatContext->streams[m_iVideoIndex];
		//auto tb = pStream->time_base;
		//auto tb0 = pStream->codec->time_base;
		//auto tb1 = av_codec_get_pkt_timebase(pStream->codec);
		//auto stream_all_time = pStream->duration*av_q2d(tb);
		//auto stream_start_time = pStream->start_time*av_q2d(tb);
		//auto fr1 = pStream->codec->framerate;

		auto fr = av_guess_frame_rate(m_pFormatContext, pStream, NULL);
		return av_q2d(fr);
	}

	return 0;
}

std::pair<int, int> CSimpleDecoder::GetFrameSize()
{
	if (m_iVideoIndex != -1)
	{
		return std::make_pair(m_pVCodecContext->width, m_pVCodecContext->height);
	}
	return std::make_pair(0, 0);
}

int CSimpleDecoder::GetSampleRate()
{
	if (m_iAudioIndex != -1)
	{
		return m_pACodecContext->sample_rate;
	}
	return 0;
}

void CSimpleDecoder::ReverseCurrentImage()
{
	int n = m_pVFrame->height / 2;
	uint8_t* p1, *p2;

	for (int i = 0; i < n; ++i)
	{
		p1 = m_aVideoOutBuf[0] + i*m_aVideoOutLines[0];
		p2 = m_aVideoOutBuf[0] + (m_pVFrame->height - 1 - i)*m_aVideoOutLines[0];
		memcpy(m_pLineForReverse, p1, m_aVideoOutLines[0]);
		memcpy(p1, p2, m_aVideoOutLines[0]);
		memcpy(p2, m_pLineForReverse, m_aVideoOutLines[0]);
	}
}

int CDecodeLoop::Run()
{
	MSG msg;

	while (m_bRunning)
	{
		if (PeekMessage(&msg, (HWND)-1, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				m_bRunning = false;
				break;
			}
			HandleQueueMessage(msg);
		}

		if (!EndOfFile())
		{
			if (HasVideo())
			{
				m_videoLock.Lock();
				CacheImageData();
				m_videoLock.UnLock();
			}
			if (HasAudio())
			{
				m_audioLock.Lock();
				CacheAudioData();
				m_audioLock.UnLock();
			}
		}
		Sleep(7);
	}

	return 0;
}

bool CDecodeLoop::Init()
{
	if (HasAudio())
	{
		m_pSoundBuf.reset(new RingBuffer(this->m_outAudioParams.sample_rate * 4 * 3));
	}
	if (HasVideo())
	{
		int imageSize = av_image_get_buffer_size(AV_PIX_FMT_BGR24, m_pVCodecContext->width, m_pVCodecContext->height, 4);
		try
		{
			m_pImageBuf.reset(new RingBuffer((sizeof(FrameInfo) + imageSize) * 3));
		}
		catch (std::exception& e)
		{
			TRACE1("create image buffer: %s.(decode loop)", e.what());
			return false;
		}
	}
	m_iCachedImageCount = 0;
	return CMessageLoop::Init();
}

void CDecodeLoop::Destroy()
{
	m_iInterrupt = 1;
	CMessageLoop::Destroy();
	if (WaitForSingleObject(m_hThread, 200) == WAIT_TIMEOUT)
	{
		TerminateThread(m_hThread, -1);
	}
	Clean();
	m_iInterrupt = 0;
}

bool CDecodeLoop::SeekTime(double pos, double currPos)
{
	CLockGuard<CSimpleLock> guard1(&m_videoLock);
	CLockGuard<CSimpleLock> guard(&m_audioLock);

	if (m_pSoundBuf)m_pSoundBuf->Reset();
	if (m_pImageBuf)m_pImageBuf->Reset();
	m_iCachedImageCount = 0;

	if (CSimpleDecoder::SeekTime(pos, currPos))
	{
		int n;
		if (HasAudio())
		{
			if (!DecodeAudio(m_pSoundBuf.get(), n))
			{
				return false;
			}

			if (m_dCurrentAudioPts < 0)
			{
				m_dCurrentAudioPts = pos;
			}
			TRACE1("seek offset %lfaaaa\n", m_dCurrentAudioPts - pos);
		}
		if (HasVideo())
		{
			if (!DecodeVideo(NULL, m_frameDump))
			{
				return false;
			}
			TRACE1("seek offset %lfvvvv\n", m_frameDump.pts - pos);
			CacheImageData();
		}
		return true;
	}
	return false;
}

bool CDecodeLoop::EndOfAudio()
{
	return m_bEndOf && m_AudioPacket.empty() && m_pSoundBuf->IsEmpty();
}

bool CDecodeLoop::EndOfVideo()
{
	return m_bEndOf && m_VideoPacket.empty() && m_pImageBuf->IsEmpty();
}

int CDecodeLoop::GetAudioData(RingBuffer* pBuf, int want)
{
	assert(pBuf && want > 0);
	CLockGuard<CSimpleLock> guard(&m_audioLock);
	int done(00);

	assert(m_pSoundBuf);
	if (!m_pSoundBuf->IsEmpty())
	{
		done = m_pSoundBuf->TransferData(pBuf, want);
	}
	return done;
}

int CDecodeLoop::GetImageData(RingBuffer* pBuf, FrameInfo& info)
{
	CLockGuard<CSimpleLock> guard(&m_videoLock);

	if (m_iCachedImageCount <1)
	{
		TRACE("no cached imaged\n");
		return 0;
	}
	m_pImageBuf->SaveIndexState();
	if (sizeof(FrameInfo) != m_pImageBuf->ReadData((char*)&info, sizeof(FrameInfo)))
		goto Error;

	pBuf->SaveIndexState();
	if (info.dataSize != m_pImageBuf->TransferData(pBuf, info.dataSize))
	{
		pBuf->RestoreIndexState();
		goto Error;
	}
	--m_iCachedImageCount;
	return info.dataSize;
Error:
	m_pImageBuf->RestoreIndexState();
	return 0;
}

__forceinline void CDecodeLoop::CacheAudioData()
{
	int n;
	if (m_pSoundBuf->ReadableBufferLen() > m_pSoundBuf->TotalBufferLen()/2)
	{
		return;
	}
	if (DecodeAudio(m_pSoundBuf.get(), n))
	{
		return;
	}
}

__forceinline void CDecodeLoop::CacheImageData()
{
	if (m_iCachedImageCount >= 3)
		return;
	if (!DecodeVideo(NULL, m_frameDump))
	{
		return;
	}
	if (m_frameDump.dataSize+sizeof(FrameInfo) > m_pImageBuf->WriteableBufferLen())
	{
		if (!m_pImageBuf->Resize(m_pImageBuf->TotalBufferLen() + m_frameDump.dataSize + sizeof(FrameInfo)))
			return;
	}

	m_pImageBuf->SaveIndexState();
	if (sizeof(FrameInfo) != m_pImageBuf->WriteData((char*)&m_frameDump, sizeof(FrameInfo)))
		goto Error;
	if (!DecodeVideo(m_pImageBuf.get(), m_frameDump))
		goto Error;
	++m_iCachedImageCount;
	return;
Error:
	m_pImageBuf->RestoreIndexState();
}