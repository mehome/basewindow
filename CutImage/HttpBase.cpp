
#include <assert.h>
#include <string>
#include "HttpBase.h"
#include "Log.h"

HttpGetBase::HttpGetBase(const std::string& url) : url_(url),
	handleNet_(NULL),
	handleConn_(NULL), 
	handleReq_(NULL),
	uiWinINetCode_(0), 
	uiHttpCode_(0) 
{
}

HttpGetBase::HttpGetBase(const std::wstring& url)
{
	int len=WideCharToMultiByte(CP_ACP, 0, url.c_str(), url.length(), NULL, 0, NULL, NULL);
	std::unique_ptr<char[]> res(new char[len + 1]);
	res[len] = 0;
	WideCharToMultiByte(CP_ACP, 0, url.c_str(), url.length(), res.get(), len + 1, NULL, NULL);
	url_ = res.get();

	handleConn_ = handleNet_ = handleReq_ = 0;
	uiHttpCode_ = uiWinINetCode_ = 0;
}

HttpGetBase::~HttpGetBase() 
{
	if (handleReq_)
	{
		InternetCloseHandle(handleReq_);
	}
	if (handleConn_)
	{
		InternetCloseHandle(handleConn_);
	}
	if (handleNet_)
	{
		InternetCloseHandle(handleNet_);
	}
}

bool HttpGetBase::SendRequest()
{
	if (!RequestInit())
	{
		return false;
	}
	return ReadData();
}

bool HttpGetBase::RequestInit()
{
	assert(!url_.empty());
	if (handleReq_)
	{
		return false;
	}

	DWORD dwFlag;
	InternetGetConnectedState(&dwFlag, 0);
	if (dwFlag & INTERNET_CONNECTION_OFFLINE)
	{
		return false;
	}

	char host[512] = { 0 };
	char path[512] = { 0 };
	URL_COMPONENTSA urlComponents;

	memset(&urlComponents, 0, sizeof(URL_COMPONENTS));
	urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
	urlComponents.lpszExtraInfo = NULL;
	urlComponents.dwExtraInfoLength = 0;
	urlComponents.lpszHostName = host;
	urlComponents.dwHostNameLength = 512 - 1;
	urlComponents.lpszPassword = NULL;
	urlComponents.dwPasswordLength = 0;
	urlComponents.lpszScheme = NULL;
	urlComponents.dwSchemeLength = 0;
	urlComponents.lpszUrlPath = path;
	urlComponents.dwUrlPathLength = 512 - 1;

	// https://msdn.microsoft.com/en-us/library/aa920361.aspx
	if (!InternetCrackUrlA(url_.c_str(), url_.length(), ICU_ESCAPE, &urlComponents))
	{
		uiWinINetCode_ = GetLastError();
		TRACE2("%s %u\n", "InternetCrackUrl ", uiWinINetCode_);
		return false;
	}

	handleNet_ = InternetOpenA("DownloadFile", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!handleNet_)
	{
		uiWinINetCode_ = GetLastError();
		TRACE2("%s %u\n", "InternetOpen ", uiWinINetCode_);
		return false;
	}
	handleConn_ = InternetConnectA(handleNet_, host, urlComponents.nPort, "", "", INTERNET_SERVICE_HTTP, 0, 0);
	if (!handleConn_)
	{
		uiWinINetCode_ = GetLastError();
		TRACE2("%s %u\n", "InternetConnect ", uiWinINetCode_);
		return false;
	}
	handleReq_ = HttpOpenRequestA(handleConn_, "GET", path, HTTP_VERSIONA,
		NULL,
		0,
		INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD,
		0);
	if (!handleReq_)
	{
		uiWinINetCode_ = GetLastError();
		TRACE2("%s %u\n", "HttpOpenRequest ", uiWinINetCode_);
		return false;
	}

	if (HttpSendRequestA(handleReq_, NULL, 0, 0, 0))
	{
		DWORD dwCode = 0;
		DWORD dwCodeSize = sizeof(DWORD);

		if (!HttpQueryInfoA(handleReq_, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwCode, &dwCodeSize, NULL))
		{
			uiWinINetCode_ = GetLastError();
			TRACE2("%s %u\n", "HttpQueryInfo ", uiWinINetCode_);
			return false;
		}
		else
		{
			uiHttpCode_ = dwCode;
			TRACE2("%s %u\n", "http code ", uiHttpCode_);
			if (dwCode != HTTP_STATUS_OK)
			{
				TRACE("http code != HTTP_STATUS_OK\n");
				return false;
			}
		}
	}
	else
	{
		uiWinINetCode_ = GetLastError();
		TRACE2("%s %u\n", "HttpSendRequest ", uiWinINetCode_);
		return false;
	}

	return true;
}

bool HttpGetBase::ReadData()
{
	if (!handleReq_ || !handleConn_ || !handleNet_)
	{
		return false;
	}

	const DWORD kBUFLENGTH = 4096;
	char buf[kBUFLENGTH] = { 0 };
	DWORD dwWant(kBUFLENGTH);
	DWORD dwReaded;

	while (1)
	{
		if (!InternetReadFile(handleReq_, buf , dwWant, &dwReaded))
		{
			uiWinINetCode_ = GetLastError();
			TRACE2("%s %u\n", L"HttpQueryInfo ", uiWinINetCode_);
			return false;
		}

		if (readCallback_)
		{
			try
			{
				readCallback_(buf, dwReaded);
			}
			catch (...)
			{

				return false;
			}
		}
		if (dwReaded == 0)
		{
			break;
		}
	}

	return true;
}

HttpDownloadFile::HttpDownloadFile(const std::wstring& url, const std::wstring& fileName) :HttpGetBase(url)
{
	fileWrite_.open(fileName.c_str(), std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
	SetReadCallback(std::bind(&HttpDownloadFile::SaveFileData, this, std::placeholders::_1, std::placeholders::_2));
}

HttpDownloadFile::~HttpDownloadFile()
{
	if (fileWrite_.is_open())
		fileWrite_.close();
}

void HttpDownloadFile::SaveFileData(char *buf, int len)
{
	if (fileWrite_.is_open())
	{
		if (len < 1)
		{
			fileWrite_.close();
			return;
		}
		fileWrite_.write(buf, len);
	}
}

std::unique_ptr<char[]> Utf16_8(const wchar_t *pSrc)
{
	int len(0), i;
	const wchar_t *p;
	wchar_t wch;
	std::unique_ptr<char[]> pDest;

	if (!pSrc)
		return  pDest;
	p = pSrc;
	while (1)
	{
		wch = *p++;
		if (wch == L'\0')
			break;
		if (wch < 0x0080)
			++len;
		else if (wch < 0x0800)
			len += 2;
		else
			len += 3;
	}

	pDest.reset(new char[len + 1]);
	pDest[len] = '\0';

	i = -1;
	p = pSrc;
	while (1)
	{
		wch = *(p++);
		if (wch == L'\0')
			break;
		if (wch < 0x0080)
			pDest[++i] = (unsigned char)wch;
		else if (wch < 0x0800)
		{
			pDest[++i] = (unsigned char)(0xc0 | (wch >> 6));
			pDest[++i] = (unsigned char)(0x80 | (wch & 0x3f));
		}
		else
		{
			pDest[++i] = (unsigned char)(0xe0 | ((wch >> 12) & 0x0f));
			pDest[++i] = (unsigned char)(0x80 | ((wch >> 6) & 0x3f));
			pDest[++i] = (unsigned char)(0x80 | (wch & 0x3f));
		}
	}

	return pDest;
}

std::unique_ptr<wchar_t[]> Utf8_16(const char *pSrc)
{
	std::unique_ptr<wchar_t[]> pDest;
	int len(0), srcLen(0), destIndex(0), i;
	unsigned char uch, uch1, uch2;

	if (pSrc == NULL)
		return pDest;
	srcLen = strlen(pSrc);
	for (i = 0; i<srcLen;)
	{
		uch = pSrc[i];
		if (uch <= 0x7f)
			++i;
		else if (uch >= 0xc0 && uch <= 0xdf)
			i += 2;
		else if (uch >= 0xe0 && uch <= 0xef)
			i += 3;
		else
			return NULL;
		++len;
	}
	pDest.reset(new wchar_t[len + 1]);
	pDest[len] = L'\0';

	for (i = 0; i<srcLen;)
	{
		uch = pSrc[i];
		if (uch < 0x80)
		{
			pDest[destIndex++] = pSrc[i];
			++i;
		}
		else if (uch >= 0xc0 && uch <= 0xdf)
		{
			uch1 = pSrc[i + 1];
			pDest[destIndex++] = ((uch & 0x1c) << 11) +
				((uch & 0x03) << 6) + (uch1 & 0x3f);
			i += 2;
		}
		else if (uch >= 0xe0 && uch <= 0xef)
		{
			uch1 = pSrc[i + 1];
			uch2 = pSrc[i + 2];
			pDest[destIndex++] = ((uch & 0x0f) << 12) + ((uch1 & 0x3c) << 6) +
				((uch1 & 0x03) << 6) + (uch2 & 0x3f);
			i += 3;
		}
	}

	return pDest;
}

std::wstring GetTempUniqueName()
{
	TCHAR buf[1024] = { 0 };
	GetTempPath(1024, buf);

	GetTempFileName(buf, NULL, 0, buf);
	return buf;
}

JsonValue::JsonValue(JVType t) :type_(t)
{
	switch (type_)
	{
	case JVString:
		value_.szValue_ = new std::wstring();
		break;
	case JVObject:
		value_.pObjValue_ = new std::vector<std::pair<JsonValue, JsonValue>>();
		break;
	case JVArray:
		value_.pArrayValue_ = new std::vector<JsonValue>();
		break;
	default:
		value_.iValue_ = 0;
		break;
	}
}

const JsonValue & JsonValue::operator=(const JsonValue &r)
{
	if (this == &r)
	{
		return *this;
	}
	Clear();
	type_ = r.type_;
	switch (type_)
	{
	case JVString:
		value_.szValue_ = new std::wstring(*r.value_.szValue_);
		break;
	case JVObject:
		value_.pObjValue_ = new std::vector<std::pair<JsonValue, JsonValue>>
			(*r.value_.pObjValue_);
		break;
	case JVArray:
		value_.pArrayValue_ = new std::vector<JsonValue>
			(*r.value_.pArrayValue_);
		break;
	default:
		value_.iValue_ = r.value_.iValue_;
		break;
	}

	return *this;
}

const JsonValue & JsonValue::operator=(JsonValue &&r)
{
	if (this == &r)
		return *this;
	Clear();
	type_ = r.type_;
	value_.iValue_ = r.value_.iValue_;
	r.type_ = JVEmpty;
	r.value_.iValue_=0;

	return *this;
}

//for object
JsonValue & JsonValue::operator[](const wchar_t* sz)
{
	if (type_ == JVObject)
	{
		auto iter = std::find_if(value_.pObjValue_->begin(),
			value_.pObjValue_->end(),
			[sz](std::pair<JsonValue, JsonValue> &i)->bool
		{
			return wcscmp(i.first.value_.szValue_->c_str(), sz)==0;
		});

		if (iter != value_.pObjValue_->end())
			return iter->second;
		else
		{
			//默认加字符串
			value_.pObjValue_->push_back(std::make_pair(JsonValue(sz), JsonValue()));
			return value_.pObjValue_->back().second;
		}
	}

	return *this;
}

//for array
JsonValue & JsonValue::operator[](int index)
{
	if (type_ == JVArray)
	{
		if (index >= (int)value_.pArrayValue_->size())
		{
			value_.pArrayValue_->push_back(JsonValue());
			return value_.pArrayValue_->back();
		}
		else
			return value_.pArrayValue_->operator[](index);
	}

	return *this;
}

void JsonValue::Clear()
{
	switch (type_)
	{
	case JVString:
		delete value_.szValue_;
		break;
	case JVObject:
		delete value_.pObjValue_;
		break;
	case JVArray:
		delete value_.pArrayValue_;
		break;
	}
}

std::wstring JsonValue::ToString(bool bAddEscape)const
{
	std::wstringstream wss;

	switch (type_)
	{
	case JVString:
		if(bAddEscape)
		{
			wchar_t ch;
			for(auto iter=value_.szValue_->begin(); iter != value_.szValue_->end(); ++iter)
			{
				ch=*iter;
				if(ch==L'"')
					wss<<'\\';
				wss<<ch;
			}
			return L'"' + wss.str() + L'"';
		}
		else
		{
			return L'"' + *value_.szValue_ + L'"';
		}
	case JVBoolean:
		return value_.bValue_ ? L"true" : L"false";
	case JVObject:
		{
			std::pair<JsonValue, JsonValue> *p;
			wss << L'{';
			for (unsigned int i = 0; i < value_.pObjValue_->size(); ++i)
			{
				p = &value_.pObjValue_->operator[](i);
				wss << p->first.ToString(bAddEscape) << L':' << p->second.ToString(bAddEscape);
				if (i != value_.pObjValue_->size() - 1)
					wss << L',';
			}
			wss << L'}';
		}
		break;
	case JVArray:
		{
			wss << L'[';
			for (unsigned int i = 0; i < value_.pArrayValue_->size(); ++i)
			{
				wss << value_.pArrayValue_->operator[](i).ToString(bAddEscape);
				if (i != value_.pArrayValue_->size() - 1)
					wss << L',';
			}
			wss << L']';
		}
		break;
	case JVInt:
		wss << value_.iValue_;
		break;
	case JVFloat:
		wss << value_.fValue_;
		break;
	case JVEmpty:
		wss << L"empty";
		break;
	}

	return wss.str().c_str();
}

std::unique_ptr<JsonValue> JsonValue::FormatByUtf16String(const wchar_t *pJT)
{
	if (!pJT)
		return NULL;
	int i, len, index(0);
	wchar_t wch;
	std::unique_ptr<wchar_t[]>pTig;
	bool bEnterQuto(false);

	len = wcslen(pJT);
	if (len < 1)
		return NULL;
	pTig.reset(new wchar_t[len]);
	for (i = 0; i < len; ++i)
	{
		wch = pJT[i];
		if(wch == L'\\' && i+1 < len && pJT[i+1] == L'"')
		{
			pTig[index++] = L'\\';
			pTig[index++] = L'\"';
			++i;
		}
		if (wch == L'\"')
			bEnterQuto = !bEnterQuto;
		if (!IsBlank(wch) || bEnterQuto)
		{
			pTig[index++] = wch;
		}
	}
	len = index;

	return FormatObject(pTig.get(), 0, len);
}

//[start,end),fit to all functions folllowing
std::unique_ptr<JsonValue> JsonValue::FormatObject(const wchar_t * pT, int start, int end)
{
	std::unique_ptr<JsonValue >pObj;
	std::pair<JsonValue, JsonValue> kv;
	int index, i;
	int countBrace(0), countBracket(0);
	bool bEnterQuto(false);
	wchar_t ch;

	if (start < 0 || end - start < 1 || pT[start] != L'{' || pT[end - 1] != L'}')
		return NULL;
	pObj.reset(new JsonValue(JVObject));

	index = start + 1;
	for (i = start + 1; i < end - 1;)
	{
		ch = pT[i];
		if(ch == L'\\' && i+1 < end-1 && pT[i+1] == L'"')
		{
			i+=2;
			continue;
		}
		else if (ch == L'"')
			bEnterQuto = !bEnterQuto;
		else if (ch == L'{')
			++countBrace;
		else if (ch == L'}')
			--countBrace;
		else if (ch == L'[')
			++countBracket;
		else if (ch == L']')
			--countBracket;

		if ((!bEnterQuto && countBrace == 0 && countBracket == 0) && ch == L',')
		{
			//"s":0,at least 5 characters
			if (i - index < 5)
				return NULL;
			kv = FormatPair(pT, index, i);
			if (kv.first.GetType() != JVString)
				return NULL;
			pObj->value_.pObjValue_->push_back(std::move(kv));
			index = i + 1;
			i = index;
		}
		else
			++i;
	}

	kv = FormatPair(pT, index, i);
	if (kv.first.GetType() != JVString)
		return NULL;
	pObj->value_.pObjValue_->push_back(std::move(kv));

	return pObj;
}

std::pair<JsonValue, JsonValue> JsonValue::FormatPair(const wchar_t *pPair, int start, int end)
{
	int i;
	JsonValue k, v;
	bool bEnterQuto(false);
	wchar_t ch;

	if (!pPair || start < 0 || end - start < 5)
		return std::make_pair(JsonValue(), JsonValue());

	for (i = start; i < end; ++i)
	{
		ch = pPair[i];
		if(ch == L'\\' && i+1 < end && pPair[i+1] == L'\"')
		{
			++i;
			continue;
		}
		else if (ch == L'"')
			bEnterQuto = !bEnterQuto;

		if (!bEnterQuto && pPair[i] == L':' && i+1<end)
		{
			k = FormatString(pPair, start, i);
			if (k.GetType() != JVString)
				return std::make_pair(JsonValue(), JsonValue());
			// 分析值的部分
			if (pPair[i + 1] == L'\"')
			{
				v = FormatString(pPair, i + 1, end);

				if (v.GetType() != JVString)
					return std::make_pair(JsonValue(JVEmpty), JsonValue(JVEmpty));
			}
			else if (pPair[i + 1] == L'{')
			{
				v = std::move(*FormatObject(pPair, i + 1, end).get());
				if (v.GetType() != JVObject)
					return std::make_pair(JsonValue(JVEmpty), JsonValue(JVEmpty));
			}
			else if (pPair[i + 1] == L'[')
			{
				v = FormatArray(pPair, i + 1, end);
				if (v.GetType() != JVArray)
					return std::make_pair(JsonValue(JVEmpty), JsonValue(JVEmpty));
			}
			else
			{
				v = FormatNumber(pPair, i + 1, end);
				if (v.GetType() != JVInt && v.GetType() != JVFloat && v.GetType() != JVBoolean)
					return std::make_pair(JsonValue(), JsonValue());
			}

			break;
		}
	}

	return std::make_pair(std::move(k), std::move(v));
}

JsonValue JsonValue::FormatArray(const wchar_t *pA, int start, int end)
{
	if (!pA || start < 0 || end - start < 2 || pA[start] != L'[' || pA[end - 1] != L']')
		return JsonValue();

	JsonValue arr(JVArray), item;
	int i, last;
	unsigned int index(0);
	int countBrace(0), countBracket(0);
	bool bEnterQuto(false);
	wchar_t ch;

	last = start + 1;
	for(i=last; i<end;)
	{
		ch = pA[i];
		if(ch == L'\\' && i+1 < end && pA[i+1] == L'"')
		{
			i+=2;
			continue;
		}
		else if (ch == L'"')
			bEnterQuto = !bEnterQuto;
		else if (ch == L'{')
			++countBrace;
		else if (ch == L'}')
			--countBrace;
		else if (ch == L'[')
			++countBracket;
		else if (ch == L']')
			--countBracket;

		if ((i == end - 1) || (!bEnterQuto && countBrace == 0 && countBracket == 0 && ch == L','))
		{
			if (pA[last] == L'"')
			{
				item = FormatString(pA, last, i);
				if (item.GetType() != JVString)
					return JsonValue();
				arr[index++] = std::move(item);
			}
			else if (pA[last] == L'[')
			{
				item = FormatArray(pA, last, i);
				if (item.GetType() != JVArray)
					return JsonValue();
				arr[index++] = std::move(item);
			}
			else if (pA[last] == L'{')
			{
				auto subObj = FormatObject(pA, last, i);
				item = std::move(*subObj);
				if (item.GetType() != JVObject)
					return JsonValue();
				arr[index++] = std::move(item);
			}
			else
			{
				item = FormatNumber(pA, last, i);
				if (item.GetType() != JVInt && item.GetType() != JVFloat && item.GetType() != JVBoolean)
					return JsonValue();
				arr[index++] = std::move(item);
			}

			last = i + 1;
			i = last;
		}
		else
			++i;
	}

	return arr;
}

JsonValue JsonValue::FormatNumber(const wchar_t *pNT, int start, int end)
{
	bool bfloat(false);
	bool bNeg(false);
	std::wstringstream wss;
	wchar_t ch;

	if (!pNT || start < 0 || end - start < 1)
		return JsonValue();
	ch = pNT[start];
	if (ch == L'-')
	{
		//负数
		bNeg = true;
		if (start + 1 == end)
			return JsonValue();
		++start;
	}
	else if (ch == L't' || ch == L'T' || ch == L'f' || ch == 'F')
	{
		//布尔值
		if (end - start != 4)
			return JsonValue();
		wchar_t a[5] = L"\0";
		memcpy(a, pNT + start, sizeof(wchar_t) * 4);
		std::for_each(a, a + 4, [](wchar_t & wch) {
			if (wch >= L'A' && wch <= L'Z')
				wch = L'a' + wch - L'A';
		});

		if (wcscmp(a, L"true") == 0)
			return JsonValue(true);
		else if (wcscmp(a, L"false") == 0)
			return JsonValue(false);

		return JsonValue();
	}
	else if (ch <L'0' && ch > L'9')
		return JsonValue();

	for (int i = start; i < end; ++i)
	{
		//符号.是允许的
		if (pNT[i] == L'.')
		{
			if (bfloat)
				return JsonValue();
			bfloat = true;
			wss << pNT[i];
			continue;
		}
		//只接受十进制数字对应的字符
		if (pNT[i] < L'0' || pNT[i] >L'9')
			return JsonValue();
		wss << pNT[i];
	}

	if (bfloat)
	{
		float f(0);
		wss >> f;
		if (bNeg)
			f = -f;
		return JsonValue(f);
	}
	else
	{
		int n(0);
		wss >> n;
		if (bNeg)
			n = -n;
		return JsonValue(n);
	}
}

JsonValue JsonValue::FormatString(const wchar_t *pKey, int start, int end)
{
	if (!pKey || start < 0 || end - start < 2)
		return JsonValue(JVEmpty);
	if (pKey[start] != L'\"' || pKey[end - 1] != L'\"')
		return JsonValue(JVEmpty);

	wchar_t wch, wchu;
	wchar_t wcha[5] = { 0 };
	std::wstringstream wss;
	for (int i = start + 1; i < end - 1; ++i)
	{
		wch = *(pKey + i);
		if (wch == L'\\' && i + 1 < end - 1)
		{
			wchu = *(pKey + i + 1);
			if ((wchu == L'u' || wchu == L'U') && i + 5 < end - 1)
			{
				memcpy(wcha, pKey + i + 2, sizeof(wchar_t) * 4);
				wchu = (wchar_t)wcstoul(wcha, NULL, 16);
				wss << wchu;
				i += 5;
				continue;
			}
			else if(wchu == L'"')
			{
				wss << L'"';
				++i;
				continue;
			}
		}
		wss << wch;
	}

	return JsonValue(wss.str().c_str());
}

bool JsonValue::IsBlank(wchar_t wch)
{
	//空格 制表 回车 换行
	return wch == L' ' || wch == L'	' || wch == L'\r' || wch == L'\n';
}