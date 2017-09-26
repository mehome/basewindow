#include "AVDecodeMulti.h"
#include <cassert>

AVDecodeMulti::AVDecodeMulti():
	m_dSeekTargetPos(0),
	m_dSeekCurrPos(0),
	m_iSeekRequest(0),
	m_iFlushVideoBufRequest(0),
	m_iFlushAudioBufRequest(0)
{
}

AVDecodeMulti::~AVDecodeMulti()
{
}

bool AVDecodeMulti::Init()
{
	if (m_threadAudio.joinable() || m_threadVideo.joinable() || m_threadRead.joinable())
	{
		return false;
	}

	InitAVDataBuffer(2, 3);

	m_bRunning = true;
	m_threadRead = std::thread(std::bind(&AVDecodeMulti::ReadThread, this));
	if(HasAudio())
		m_threadAudio = std::thread(std::bind(&AVDecodeMulti::AudioThread, this));
	if(HasVideo())
		m_threadVideo = std::thread(std::bind(&AVDecodeMulti::VideoThread, this));

	return true;
}

void AVDecodeMulti::Destroy()
{
	m_bRunning = false;
	if (m_threadAudio.joinable())m_threadAudio.join();
	if (m_threadVideo.joinable())m_threadVideo.join();
	if (m_threadRead.joinable())m_threadRead.join();
	
}

bool AVDecodeMulti::SeekTime(double target_pos, double currPos)
{
	m_iSeekRequest = 2;
	m_dSeekTargetPos = target_pos;
	m_dSeekCurrPos = currPos;

	return true;
}

void AVDecodeMulti::ReadThread()
{
	AVPacket packet;

	while (m_bRunning)
	{
		if (m_iSeekRequest > 0)
		{
			m_iSeekRequest = 0;

			int64_t minpts, maxpts, ts;
			ts = (int64_t)(m_dSeekTargetPos*AV_TIME_BASE);
			if (m_dSeekCurrPos >= m_dSeekTargetPos)
			{
				minpts = INT64_MIN;
				maxpts = (int64_t)(m_dSeekCurrPos*AV_TIME_BASE - 2);
			}
			else
			{
				minpts = (int64_t)(m_dSeekCurrPos*AV_TIME_BASE + 2);
				maxpts = INT64_MAX;
			}
			auto res = avformat_seek_file(m_pFormatContext, -1, minpts, ts, maxpts, 0);
			if (res < 0)
			{
				m_bRunning = false;
				break;
			}

			AVPacket temp{ 0 };
			temp.stream_index = -1;

			if (HasVideo())
			{
				m_videoPackets.m_mutexPackets.lock();
				m_videoPackets.ClearPacketQueue();
				m_videoPackets.AddPacketBack(temp);
				m_videoPackets.m_cvPackets.notify_one();
				m_iFlushVideoBufRequest = 2;
				m_videoPackets.m_mutexPackets.unlock();
			}

			if (HasAudio())
			{
				m_audioPackets.m_mutexPackets.lock();
				m_audioPackets.ClearPacketQueue();
				m_audioPackets.AddPacketBack(temp);
				m_audioPackets.m_cvPackets.notify_one();
				m_iFlushAudioBufRequest = 2;
				m_audioPackets.m_mutexPackets.unlock();
			}
			continue;
		}

		if (m_videoPackets.GetPacketDurationTime() > 1.0 && m_audioPackets.GetPacketDurationTime() > 1.0)
		{
			std::unique_lock<std::mutex> guard(m_mutexRead);
			m_cvRead.wait_for(guard, std::chrono::milliseconds(10));
			continue;
		}

		if (ReadPacket(&packet) < 0)
		{
			//m_bRunning = false;
			break;
		}

		if (packet.stream_index == m_iVideoIndex)
		{
			m_videoPackets.m_mutexPackets.lock();
			m_videoPackets.AddPacketBack(packet);
			m_videoPackets.m_cvPackets.notify_one();
			m_videoPackets.m_mutexPackets.unlock();
		}
		else if (packet.stream_index == m_iAudioIndex)
		{
			m_audioPackets.m_mutexPackets.lock();
			m_audioPackets.AddPacketBack(packet);
			m_audioPackets.m_cvPackets.notify_one();
			m_audioPackets.m_mutexPackets.unlock();
		}
		else
		{
			av_packet_unref(&packet);
			continue;
		}

	}
}

void AVDecodeMulti::AudioThread()
{
	int64_t thisFrameChannelLayout;
	int res, n, outCount, outSize;
	AVPacket packet;
	std::unique_lock<std::mutex> guard_packet(m_audioPackets.m_mutexPackets);
	std::unique_lock<std::mutex> guard_audio_buf(m_audioMutex);
	guard_packet.unlock();
	guard_audio_buf.unlock();

	while (m_bRunning)
	{
		if (m_iAudioOutRemaind != 0)
		{
			guard_audio_buf.lock();
			if (m_pSoundBuf->WriteableBufferLen() < m_iAudioOutRemaind)
			{
				if (m_iFlushAudioBufRequest == 2)
				{
					m_iAudioOutRemaind = 0;
				}
				else if (m_iFlushAudioBufRequest == 1)
				{
					m_iFlushAudioBufRequest = 0;
					if (m_iFlushVideoBufRequest == 0 && m_funcSeekCallback)
					{
						guard_audio_buf.unlock();
						m_funcSeekCallback();
						continue;
					}
				}
				else
				{
					m_audioCV.wait_for(guard_audio_buf, std::chrono::milliseconds(10));
				}
				guard_audio_buf.unlock();
				continue;
			}

			n = m_pSoundBuf->WriteData((char*)m_pAudioOutBuf, m_iAudioOutRemaind);
			if (n != m_iAudioOutRemaind)
			{
				m_iAudioOutRemaind -= n;
				memmove(m_pAudioOutBuf, m_pAudioOutBuf + n, m_iAudioOutRemaind);
			}
			else
			{
				m_iAudioOutRemaind = 0;
			}

			guard_audio_buf.unlock();
			continue;
		}

		guard_packet.lock();
		if (m_audioPackets.m_packets.empty())
		{
			m_cvRead.notify_one();
			m_audioPackets.m_cvPackets.wait_for(guard_packet, std::chrono::milliseconds(10));
			guard_packet.unlock();
			continue;
		}
		m_audioPackets.PeekPacket(packet);
		if (packet.stream_index == -1)
		{
			guard_audio_buf.lock();
			m_pSoundBuf->Reset();
			m_iFlushAudioBufRequest = 1;
			m_dCurrentAudioPts = -0.0001;
			guard_audio_buf.unlock();
			guard_packet.unlock();
			avcodec_flush_buffers(m_pACodecContext);
			continue;
		}
		guard_packet.unlock();

		res = avcodec_send_packet(m_pACodecContext, &packet);
		if (res == 0)
		{
			av_packet_unref(&packet);
		}
		else
		{
			if (res == AVERROR(EAGAIN))
			{
				guard_packet.lock();
				m_audioPackets.AddPacketFront(packet);
				guard_packet.unlock();
			}
			else
			{
				av_packet_unref(&packet);
				continue;
			}
		}

		res = avcodec_receive_frame(m_pACodecContext, m_pAFrame);
		if (res == 0)
		{
			if (m_dCurrentAudioPts < 0 && m_pAFrame->pts != AV_NOPTS_VALUE)
			{
				m_dCurrentAudioPts = m_pAFrame->pts * av_q2d(av_codec_get_pkt_timebase(m_pACodecContext));
			}

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
					return;
				}
			}

			outCount = swr_get_out_samples(m_pASwr, m_pAFrame->nb_samples);
			if (outCount < 0)
			{
				return;
			}
			outSize = av_samples_get_buffer_size(NULL, m_outAudioParams.channels, outCount, m_outAudioParams.sample_fmt, 0);
			if (outSize > m_iAudioOutLen)
			{
				av_fast_malloc(&m_pAudioOutBuf, (unsigned int*)&m_iAudioOutLen, outSize);
				if (!m_pAudioOutBuf)
					return;
			}

			res = swr_convert(this->m_pASwr, &m_pAudioOutBuf, outCount, (const uint8_t**)m_pAFrame->extended_data, m_pAFrame->nb_samples);
			if (res < 0)
				return;
			outSize = av_samples_get_buffer_size(NULL, m_outAudioParams.channels, res, m_outAudioParams.sample_fmt, 1);
			m_iAudioOutRemaind = outSize;
		}
		else if (res == AVERROR(EAGAIN))
		{
			continue;
		}
		else
		{
			// ÆäËû´íÎó
			return;
		}
	}
}

void AVDecodeMulti::VideoThread()
{
	AVPacket packet;
	int res;
	uint64_t pts;
	std::unique_lock<std::mutex> guard_packet(m_videoPackets.m_mutexPackets);
	std::unique_lock<std::mutex> guard_video_buf(m_videoMutex);
	guard_packet.unlock();
	guard_video_buf.unlock();

	while (m_bRunning)
	{
		if (m_bCurrentImageNotCopy)
		{
			guard_video_buf.lock();
			if (m_pImageBuf->WriteableBufferLen() < (sizeof(FrameInfo) + m_frameDump.dataSize))
			{
				if (m_iFlushVideoBufRequest == 2)
				{
					m_bCurrentImageNotCopy = false;
				}
				else if (m_iFlushVideoBufRequest == 1)
				{
					m_iFlushVideoBufRequest = 0;
					if (m_iFlushAudioBufRequest == 0 && m_funcSeekCallback)
					{
						guard_video_buf.unlock();
						m_funcSeekCallback();
						continue;
					}
				}
				else
				{
					m_videoCV.wait_for(guard_video_buf, std::chrono::milliseconds(10));
				}
				guard_video_buf.unlock();
				continue;
			}
			m_pImageBuf->WriteData((char*)&m_frameDump, sizeof(FrameInfo));
			m_pImageBuf->WriteData((char*)m_aVideoOutBuf[0], m_frameDump.dataSize);
			m_bCurrentImageNotCopy = false;
			++m_iCachedImageCount;
			guard_video_buf.unlock();
		}

		guard_packet.lock();
		if (m_videoPackets.m_packets.empty())
		{
			m_cvRead.notify_one();
			m_videoCV.wait_for(guard_packet, std::chrono::milliseconds(10));
			guard_packet.unlock();
			continue;
		}
		m_videoPackets.PeekPacket(packet);
		if (packet.stream_index == -1)
		{
			guard_video_buf.lock();
			m_pImageBuf->Reset();
			m_iCachedImageCount = 0;
			m_iFlushVideoBufRequest = 1;
			guard_video_buf.unlock();
			guard_packet.unlock();
			avcodec_flush_buffers(m_pVCodecContext);
			continue;
		}
		guard_packet.unlock();

		res = avcodec_send_packet(m_pVCodecContext, &packet);
		if (res == 0)
		{
			av_packet_unref(&packet);
		}
		else
		{
			if (res == AVERROR(EAGAIN))
			{
				guard_packet.lock();
				m_videoPackets.AddPacketFront(packet);
				guard_packet.unlock();
				continue;
			}
			else if (res == AVERROR_INVALIDDATA)
			{
				av_packet_unref(&packet);
				continue;
			}
			else
			{
				// AVERROR(EOF) etc
				return;
			}
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
					return;
			}

			res = sws_scale(m_pVSws, m_pVFrame->data, m_pVFrame->linesize, 0, m_pVFrame->height, m_aVideoOutBuf, m_aVideoOutLines);
			assert(res == m_outVideoParams.heigh);

			m_frameDump.dataSize = av_image_get_buffer_size(m_outVideoParams.fmt, m_outVideoParams.width, res, 4);
			m_frameDump.width = m_outVideoParams.width;
			m_frameDump.height = res;
			m_frameDump.pts = m_dCurrentImagePts;
			m_bCurrentImageNotCopy = true;
		}
		else if (res == AVERROR(EAGAIN))
		{
			continue;
		}
		else
		{
			return;
		}
	}
}