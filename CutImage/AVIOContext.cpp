#include "AVIOContext.h"
#include <cassert>

extern "C"
{
#include <libavutil\avutil.h>
}

int ReadIO(void *opaque, uint8_t *buf, int buf_size)
{
	IIOContext* p = static_cast<IIOContext*>(opaque);
	return p->ReadIO(buf, buf_size);
}

int WriteIO(void *opaque, uint8_t *buf, int buf_size)
{
	assert(0);
	return 0;
}

int64_t SeekIO(void *opaque, int64_t offset, int whence)
{
	IIOContext* p = static_cast<IIOContext*>(opaque);
	return p->SeekIO(offset, whence);
}

IIOContext::IIOContext():
	m_pBuffer(0),
	m_iBufLen(0)
{
}

IIOContext::~IIOContext()
{
	if (m_pBuffer)
	{
		//m_pBuffer已经不再有效
		//av_free(&m_pBuffer); //cannot free this
	}
}

void IIOContext::CreateBuffer(int size)
{
	assert(size > 0);
	if (m_pBuffer)
	{
		av_freep(&m_pBuffer);
	}
	m_pBuffer = (unsigned char*)av_malloc(size);
	m_iBufLen = size;
	assert(m_pBuffer);
}

CFileMappingIO::CFileMappingIO(const std::string& file):
	m_hFile(NULL),
	m_hFileMapping(NULL),
	m_iCurrentPos(0),
	m_pData(NULL)
{
	m_hFile = CreateFileA(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		DWORD h(0);
		m_liFileSize.QuadPart = 0;
		m_liFileSize.LowPart = GetFileSize(m_hFile, &h);
		if (h != 0)
		{
			m_liFileSize.HighPart = static_cast<LONG>(h);
		}
		m_hFileMapping = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, m_liFileSize.HighPart, m_liFileSize.LowPart, NULL);
		if (m_hFileMapping)
		{
			m_pData = (uint8_t*)MapViewOfFile(m_hFileMapping, FILE_MAP_READ, 0, 0, (SIZE_T)(m_liFileSize.QuadPart));
		}
	}
	assert(m_pData != NULL);
}

CFileMappingIO::~CFileMappingIO()
{
	if (m_pData)
	{
		UnmapViewOfFile(m_pData);
	}
	if (m_hFileMapping)
	{
		CloseHandle(m_hFileMapping);
	}
	if (m_hFile)
	{
		CloseHandle(m_hFile);
	}
}

void CFileMappingIO::Reset()
{
	m_iCurrentPos = 0;
}

int CFileMappingIO::ReadIO(uint8_t* buf, int size)
{
	if ((LONGLONG)(size + m_iCurrentPos) <= m_liFileSize.QuadPart)
	{
		memcpy(buf, m_pData + m_iCurrentPos, size);
		m_iCurrentPos += size;
		return size;
	}
	else
	{
		size_t s = (size_t)(m_liFileSize.QuadPart - m_iCurrentPos);
		memcpy(buf, m_pData + m_iCurrentPos, s);
		m_iCurrentPos = (uint64_t)m_liFileSize.QuadPart;
		return s;
	}
}

int64_t CFileMappingIO::SeekIO(int64_t offset, int whence)
{
	//AVSEEK_SIZE
	switch (whence)
	{
	case 0:
		m_iCurrentPos = offset;
		break;
	case 1:
		if (m_iCurrentPos + offset <= m_liFileSize.QuadPart)
		{
			m_iCurrentPos += offset;
		}
		else
		{
			return -1;
		}
		break;
	case 2:
		if (m_liFileSize.QuadPart + offset >= 0)
		{
			m_iCurrentPos = m_liFileSize.QuadPart + offset;
		}
		else
		{
			return -1;
		}
		break;
	case 0x10000:
		return (int64_t)m_liFileSize.QuadPart;
	}

	return m_iCurrentPos;
}