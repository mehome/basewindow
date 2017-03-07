#pragma once
class RingBuffer
{
public:
	RingBuffer(int len, char* pOutsidebuf = 0);
	~RingBuffer();
	bool IsEmpty() { return m_iRead == m_iWrite && !m_bFull; }
	bool IsFull() { return m_bFull; }
	int TotalBufferLen() { return m_iLen; }
	int ReadableBufferLen() { return m_iUsableLen; }
	int WriteableBufferLen() { return m_iLen - m_iUsableLen; }
	int WriteData(const char* pData, int len);
	int ReadData(char* rcvbuf, int want);
protected:
	char* m_pBuf;
	int m_iLen;
	int m_iUsableLen;
	int m_iWrite;
	int m_iRead;
	bool m_bFull;
	bool m_bUseOutsideBuf;
};
