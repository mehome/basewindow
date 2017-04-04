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
	m_pData(NULL),
	m_iMapLen(0),
	m_iMapReadIndex(0)
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
		if (m_liFileSize.QuadPart != 0)
		{
			m_hFileMapping = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, m_liFileSize.HighPart, m_liFileSize.LowPart, NULL);
			if (m_hFileMapping)
			{
				MapFile(0);
			}
		}
	}
	if (!m_pData)
	{
		throw std::exception("cannot map file");
	}
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
	if (!m_pData)
		return 0;
	if ((int64_t)(size + m_iMapReadIndex) < m_iMapLen)
	{
		memcpy(buf, m_pData + m_iMapReadIndex, size);
		m_iCurrentPos += size;
		m_iMapReadIndex += size;
		return size;
	}
	else
	{
		auto s = m_iMapLen - m_iMapReadIndex;
		memcpy(buf, m_pData + m_iMapReadIndex, (size_t)s);
		m_iCurrentPos += s;
		MapFile(m_iCurrentPos);
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
			m_iCurrentPos += offset;
		else
			return -1;
		break;
	case 2:
		if (m_liFileSize.QuadPart + offset >= 0)
			m_iCurrentPos = m_liFileSize.QuadPart + offset;
		else
			return -1;
		break;
	case 0x10000:
		return (int64_t)m_liFileSize.QuadPart;
	}
	if (!MapFile(m_iCurrentPos))
		return -1;
	return m_iCurrentPos;
}

bool CFileMappingIO::MapFile(int64_t offset)
{
	LARGE_INTEGER li;
	DWORD offset_high;

	if (m_pData)
	{
		if (!UnmapViewOfFile(m_pData))
			return false;
		m_pData = NULL;
	}
	if (offset == m_liFileSize.QuadPart)
	{
		return false;
	}
	m_iMapReadIndex = 0;

	static SYSTEM_INFO si = { 0 };
	if (si.dwAllocationGranularity == 0)GetSystemInfo(&si);
	auto yu = offset%si.dwAllocationGranularity;
	if (yu)
	{
		offset -= yu;
		if (offset < 0)
		{
			yu += offset;
			offset = 0;
		}
		m_iMapReadIndex = yu;
	}

	li.QuadPart = offset;
	offset_high = (DWORD)li.HighPart;
	m_iMapLen = m_liFileSize.QuadPart - offset;
	m_iMapLen = min(si.dwAllocationGranularity * 100, m_iMapLen);
	while (!m_pData)
	{
		m_pData = (uint8_t*)MapViewOfFile(m_hFileMapping, FILE_MAP_READ, offset_high, li.LowPart, (SIZE_T)m_iMapLen);
		if (!m_pData)
		{
			auto error = GetLastError();
			if (error != 8 && error != 87)
				return  false;
			m_iMapLen /= 2;
		}
	}
	
	return true;
}