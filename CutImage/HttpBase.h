#pragma once

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <sstream>

#include <Windows.h>
#include <WinInet.h>

#pragma comment(lib, "WinInet.lib")

class HttpGetBase
{
public:
	HttpGetBase(const std::string& url);
	HttpGetBase(const std::wstring& url);
	virtual ~HttpGetBase();
	virtual bool SendRequest();
	unsigned int GetHttpCode()const
	{
		return uiHttpCode_;
	}
	unsigned int GetWinINetCode()const
	{
		return uiWinINetCode_;
	}
	void SetReadCallback(std::function<void(char *, int len)> func)
	{
		readCallback_ = func;
	}
protected:
	virtual bool RequestInit();
	virtual bool ReadData();
protected:
	unsigned int uiHttpCode_;
	unsigned int uiWinINetCode_;
	std::string url_;
	HINTERNET handleNet_;
	HINTERNET handleConn_;
	HINTERNET handleReq_;
	std::function<void(char * buf, int len)> readCallback_;
};

class HttpDownloadFile :public HttpGetBase
{
public:
	HttpDownloadFile(const std::wstring& url, const std::wstring& fileName);
	~HttpDownloadFile();
protected:
	virtual void SaveFileData(char *buf, int len);
protected:
	std::ofstream fileWrite_;
};

std::unique_ptr<char[]> Utf16_8(const wchar_t *pSrc);

std::unique_ptr<wchar_t[]> Utf8_16(const char *pSrc);

std::wstring GetTempUniqueName();

struct JsonValue
{
	enum JVType
	{
		JVEmpty,
		JVString,
		JVBoolean,
		JVObject,
		JVArray,
		JVInt,
		JVFloat
	};

	union JVValue
	{
		std::wstring *szValue_;
		int bValue_;
		std::vector<std::pair<JsonValue, JsonValue>> *pObjValue_;
		std::vector<JsonValue> *pArrayValue_;
		int iValue_;
		float fValue_;

		JVValue() :szValue_(0)
		{
		}
	};

	JsonValue() :type_(JVEmpty)
	{
	}

	explicit JsonValue(const wchar_t *pSZ) :type_(JVString)
	{
		value_.szValue_ = new std::wstring(pSZ);
	}

	explicit JsonValue(bool b) :type_(JVBoolean)
	{
		value_.bValue_ = b ? 1 : 0;
	}

	explicit JsonValue(int n) :type_(JVInt)
	{
		value_.iValue_ = n;
	}

	explicit JsonValue(float f) :type_(JVFloat)
	{
		value_.fValue_ = f;
	}

	explicit JsonValue(JVType t) :type_(t)
	{
		Init();
	}

	JsonValue(const JsonValue & r)
	{
		operator=(r);
	}

	~JsonValue()
	{
		Clear();
	}

	void operator=(const wchar_t * pSZ)
	{
		operator=(JsonValue(pSZ));
	}

	void operator=(const std::wstring &ws)
	{
		operator=(ws.c_str());
	}

	void operator=(bool b)
	{
		operator=(JsonValue(b));
	}

	void operator=(int n)
	{
		operator=(JsonValue(n));
	}

	void operator=(float f)
	{
		operator=(JsonValue(f));
	}

	JVType GetType()const
	{
		return type_;
	}

	const JVValue & GetValue()const
	{
		return value_;
	}

	const JsonValue & operator=(const JsonValue &r);

	const JsonValue & operator=(JsonValue &&r);

	//for object
	JsonValue & operator[](const std::wstring &sz);

	//for array
	JsonValue & operator[](unsigned int index);

	void Init();

	void Clear();

	std::wstring ToString();

	static std::unique_ptr<JsonValue> FormatByUtf16String(const wchar_t *pJT);

	//[start,end),fit to all functions folllowing
	static std::unique_ptr<JsonValue> FormatObject(const wchar_t * pT, int start, int end);

	static std::pair<JsonValue, JsonValue> FormatPair(const wchar_t *pPair, int start, int end);

	static JsonValue FormatArray(const wchar_t *pA, int start, int end);

	static JsonValue FormatNumber(const wchar_t *pNT, int start, int end);

	static JsonValue FormatString(const wchar_t *pKey, int start, int end);

	static bool IsBlank(wchar_t wch);
private:
	JVType type_;
	JVValue value_;
};