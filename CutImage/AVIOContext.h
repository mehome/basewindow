#pragma once
#include <Windows.h>
#include <stdint.h>
#include <string>

int ReadIO(void *opaque, uint8_t *buf, int buf_size);
int WriteIO(void *opaque, uint8_t *buf, int buf_size);
int64_t SeekIO(void *opaque, int64_t offset, int whence);

class IIOContext
{
private:
	IIOContext(IIOContext&) = delete;
	IIOContext& operator=(IIOContext&) = delete;
public:
	IIOContext();
	virtual ~IIOContext();
	virtual void Reset() = 0;
	virtual int ReadIO(uint8_t* buf, int size) = 0;
	virtual int64_t SeekIO(int64_t offset, int whence) = 0;

	void CreateBuffer(int size);

	unsigned char* Buf() { return m_pBuffer; }
	int BufLen() { return m_iBufLen; }
protected:
	unsigned char* m_pBuffer;
	int m_iBufLen;
};

class CFileMappingIO : public IIOContext
{
public:
	CFileMappingIO(const std::string& file);
	~CFileMappingIO();
	void Reset();
	int ReadIO(uint8_t* buf, int size);
	int64_t SeekIO(int64_t offset, int whence);
protected:
	HANDLE m_hFile;
	HANDLE m_hFileMapping;
	int64_t m_iCurrentPos;
	uint8_t* m_pData;
	LARGE_INTEGER m_liFileSize;
};