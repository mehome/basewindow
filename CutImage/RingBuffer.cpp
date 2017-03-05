#include "RingBuffer.h"
#include <assert.h>
#include <iostream>

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

RingBuffer::RingBuffer(int len)
	:m_iLen(len),
	m_iUsableLen(0),
	m_iWrite(0),
	m_iRead(0),
	m_bFull(false)
{
	m_pBuf = new unsigned char[m_iLen];
	assert(m_pBuf != NULL);
}


RingBuffer::~RingBuffer()
{
	if (m_pBuf)
		delete[]m_pBuf;
}

int RingBuffer::WriteData(const char* pData, int len)
{
	int n, done(0), want(len);

	if (IsFull())
		return 0;

	while (want > 0)
	{
		if (m_iWrite >= m_iRead)
		{
			n = m_iLen - m_iWrite;
			n = min(n, want);
			memcpy(m_pBuf + m_iWrite, pData + done, n);
			done += n;
			want -= n;
			m_iWrite += n;
			if (m_iWrite == m_iLen)
			{
				m_iWrite = 0;
			}
		}
		else
		{
			n = m_iRead - m_iWrite;
			n = min(n, want);
			memcpy(m_pBuf + m_iWrite, pData + done, n);
			done += n;
			want -= n;
			m_iWrite += n;
		}

		if (m_iWrite == m_iRead)
		{
			m_bFull = true;
			break;
		}
	}

	m_iUsableLen += done;
	return done;
}

int RingBuffer::ReadData(char* rcvbuf, int len)
{
	int n(0), done(0), want(len);

	if (IsEmpty())
		return 0;

	while (want > 0)
	{
		if (m_iRead >= m_iWrite)
		{
			n = m_iLen - m_iRead;
			n = min(n, want);
			memcpy(rcvbuf + done, m_pBuf + m_iRead, n);
			done += n;
			want -= n;
			m_iRead += n;
			if (m_iRead == m_iLen)
			{
				m_iRead = 0;
			}
		}
		else
		{
			n = m_iWrite - m_iRead;
			n = min(n, want);
			memcpy(rcvbuf + done, m_pBuf + m_iRead, n);
			done += n;
			want -= n;
			m_iRead += n;
		}
		m_bFull = false;
		if (m_iRead == m_iWrite)
			break;
	}

	m_iUsableLen -= done;
	return done;
}