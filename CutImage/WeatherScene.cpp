#include "WeatherScene.h"
#include <tchar.h>

bool WeatherScene::Init()
{
	m_szCityName=GetCity();
	//m_szCityName = L"北京市";
	if(m_szCityName.empty())
	{
		return false;
	}
	if(m_szCityName.back() == L'市')
	{
		m_szCityName=m_szCityName.substr(0, m_szCityName.length()-1);
	}
	if(!GetWeather(m_szCityName))
	{
		return false;
	}

	bool bRes(true);
	RECT r=GetRect();

	//背景图
	{
		TCHAR buf[512]={0};
		std::wstring path;
		GetModuleFileName(NULL, buf, 512);
		path=buf;
		path=path.substr(0, path.find_last_of(L'\\'));
		path+=L"\\weather.jpg";

		CStaticImageNode* pBg=new CStaticImageNode(PresentFillNode, this);
		CImageLayer* pImage=new CImageLayer(pBg);
		if(pImage->CreateImageLayerByFile(path))
		{
			pBg->SetRect(GetRect());
			pBg->SetImageLayer(pImage);
		}
		else
		{
			delete pBg;
		}
	}
	
	RECT rect=r;
	{
		LOGFONT lg;
		JsonValue& today = m_jsonWeather[0];

		memset(&lg, 0, sizeof(LOGFONT));
		_tcscpy_s(lg.lfFaceName, 32, _T("幼圆"));
		lg.lfHeight = 25;
		lg.lfWeight = FW_THIN;
		lg.lfCharSet = GB2312_CHARSET;
		lg.lfOutPrecision = OUT_DEFAULT_PRECIS;
		lg.lfClipPrecision = CLIP_CHARACTER_PRECIS;
		lg.lfQuality = ANTIALIASED_QUALITY;
		lg.lfPitchAndFamily = FF_MODERN;

		// 实时天气数据在日期字符串里面
		rect.left += 15;
		rect.top += 15;
		rect.bottom = rect.top + 25;
		JsonValue& temp = today[L"date"];
		if (temp.GetType() != JsonValue::JVString)
		{
			bRes = false;
			goto End;
		}
		std::wstring& date = *temp.GetValue().szValue_;
		size_t pos1 = date.find_first_of(L'：');
		size_t pos2 = date.find_first_of(L'℃');
		if (pos1 == std::wstring::npos || pos2 == std::wstring::npos)
		{
			bRes = false;
			goto End;
		}
		CTextLayer* pL=new CTextLayer(this);
		pL->SetText(date.substr(pos1+1, pos2-pos1), false);
		pL->SetFont(lg);
		pL->SetRect(rect);
		pL->SetAlignment(DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		//城市名
		lg.lfHeight = 24;
		lg.lfWeight = FW_NORMAL;
		lg.lfQuality = DEFAULT_QUALITY;
		_tcscpy_s(lg.lfFaceName, 32, _T("Segoe UI"));
		rect.top = rect.bottom + 1;
		rect.bottom = rect.top + 25;
		pL=new CTextLayer(this);
		pL->SetText(m_szCityName, false);
		pL->SetFont(lg);
		pL->SetRect(rect);
		pL->SetAlignment(DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		//天气
		rect.top = rect.bottom + 1;
		rect.bottom = rect.top + 25;
		JsonValue& weather = today[L"weather"];
		if (weather.GetType() != JsonValue::JVString)
		{
			bRes = false;
			goto End;
		}
		pL=new CTextLayer(this);
		pL->SetText(*weather.GetValue().szValue_);
		pL->SetFont(lg);
		pL->SetTextColor(RGB(20, 20, 20));
		pL->SetRect(rect);
		pL->SetAlignment(DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		//风向
		RECT rect_temp = pL->GetRect();
		auto size = pL->GetTextSize();
		
		rect_temp.left += size.cx + 5;
		JsonValue& wind = today[L"wind"];
		if (wind.GetType() != JsonValue::JVString)
		{
			bRes = false;
			goto End;
		}
		pL = new CTextLayer(this);
		pL->SetText(*wind.GetValue().szValue_);
		pL->SetFont(lg);
		pL->SetTextColor(RGB(20, 20, 20));
		pL->SetRect(rect_temp);
		pL->SetAlignment(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		AddChild(pL);

		//PM2.5
		rect.top = rect.bottom + 1;
		rect.bottom = rect.top + 30;
		std::wstring pm25 = L"PM2.5 " + *m_jsonPM25.GetValue().szValue_;
		pL = new CTextLayer(this);
		pL->SetText(pm25);
		pL->SetFont(lg);
		pL->SetTextColor(RGB(20, 20, 20));
		pL->SetRect(rect);
		pL->SetAlignment(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	}

	CImageLayer* pLine=new CImageLayer(this);
	pLine->CreateImageLayerByColor(80, 80, 80, 180);
	pLine->SetPos(GetSize().first/2, rect.bottom +3.0f);
	pLine->SetSize(GetSize().first, 2.0f);

	//近三日概况
	{
		rect.top = rect.bottom + 5;
		rect.bottom = r.bottom;
		int n = (rect.right - rect.left) / 3;

		for (int i = 0; i < 3; ++i)
		{
			JsonValue& day = m_jsonWeather[i];
			if (day.GetType() != JsonValue::JVObject)
			{
				bRes = false;
				goto End;
			}

			std::wstring& sURL = *day[L"dayPictureUrl"].GetValue().szValue_;
			std::wstring  sName = GetTempUniqueName();
			HttpDownloadFile download(sURL, sName);

			if (!download.SendRequest())
			{
				bRes = false;
				goto End;
			}

			std::wstring& sTemp = *day[L"temperature"].GetValue().szValue_;
			size_t pos = sTemp.find_first_of(L'~');
			if (pos == std::wstring::npos)
			{
				bRes = false;
				goto End;
			}
			std::wstring sHigh = sTemp.substr(0, pos);
			std::wstring sLow = sTemp.substr(pos + 1, sTemp.length() - pos - 2);
			int high = _wtoi(sHigh.c_str());
			int low = _wtoi(sLow.c_str());

			std::wstring sDate;
			if (i == 0) sDate = L"今天";
			else if (i == 1) sDate = L"明天";
			else if (i == 2)sDate = L"后天";

			WeatherDayGeneral *pDay = new WeatherDayGeneral(sDate, low, high, sName, this);
			rect.left = i*n;
			rect.right = rect.left + n;
			pDay->SetRect(rect);
		}
	}

	bRes=CScene::Init();
End:
	return bRes;
}

std::wstring WeatherScene::GetCity()
{
	char buf[4096] = { 0 };
	int n(0);
	HttpGetBase query("http://api.map.baidu.com/location/ip?ak=NPay7Sc8azrWV62P5THzv9BM");

	query.SetReadCallback([&](void *pData, int len) {
		memcpy(buf+n, pData, len);
		n+=len;
	});
	if (!query.SendRequest())
	{
		return L"";
	}

	auto wide_string = Utf8_16(buf);
	auto json = JsonValue::FormatByUtf16String(wide_string.get());
	if (!json)
	{
		return L"";
	}
	JsonValue& city = (*json)[L"content"][L"address_detail"][L"city"];
	if (city.GetType() != JsonValue::JVString)
	{
		return L"";
	}

	return *city.GetValue().szValue_;
}

bool WeatherScene::GetWeather(const std::wstring& cityName)
{
	if (cityName.empty())
		return false;

	int readed(0);
	char buf[10240] = { 0 };
	wchar_t wbuf[10240] = { 0 };
	swprintf_s(wbuf, 10240, L"http://api.map.baidu.com/telematics/v3/weather?location=%s"
		L"&output=json"
		L"&ak=NPay7Sc8azrWV62P5THzv9BM",
		cityName.c_str());

	// 需要utf8格式的
	auto pURL = Utf16_8(wbuf);
	HttpGetBase query(pURL.get());
	query.SetReadCallback([&](void *pData, int len) {
		memcpy(buf + readed, pData, len);
		readed += len;
	});
	if (!query.SendRequest())
	{
		return false;
	}

	auto json_string = Utf8_16(buf);
	auto json = JsonValue::FormatByUtf16String(json_string.get());

	if (!json)
	{
		// 失败
		return false;
	}
	JsonValue& error = (*json)[L"error"];
	if (error.GetType() != JsonValue::JVInt || error.GetValue().iValue_ != 0)
	{
		// 失败
		return false;
	}

	JsonValue& pm25 = (*json)[L"results"][0][L"pm25"];
	JsonValue& weather = (*json)[L"results"][0][L"weather_data"];
	m_jsonWeather = std::move(weather);
	m_jsonPM25 = std::move(pm25);

	return true;
}

WeatherDayGeneral::WeatherDayGeneral(const std::wstring& date,
	int low,
	int high,
	const std::wstring& pic,
	CNode* pParent):CNode(pParent),
	sDate_(date),
	lowT_(low),
	highT_(high),
	sPic_(pic)
{
}

bool WeatherDayGeneral::Init()
{
	LOGFONT lg;

	memset(&lg, 0, sizeof(LOGFONT));
	_tcscpy_s(lg.lfFaceName, 32, _T("微软雅黑"));
	lg.lfHeight = 20;
	lg.lfWeight = FW_THIN;
	lg.lfCharSet = GB2312_CHARSET;
	lg.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lg.lfClipPrecision = CLIP_CHARACTER_PRECIS;
	lg.lfQuality = DEFAULT_QUALITY;
	lg.lfPitchAndFamily = FF_MODERN;

	int y = 0;
	SIZE size;
	CTextLayer* pL = new CTextLayer(this);
	pL->SetText(sDate_);
	pL->SetAnchor(0, 0);
	pL->SetFont(lg);
	pL->SetTextColor(RGB(40, 40, 40));
	auto sizePair = pL->GetTextSize();
	size.cx = sizePair.cx;
	size.cy = sizePair.cy;
	pL->SetPos(0, y);
	pL->SetSize(GetSize().first, (float)size.cy);
	y += size.cy + 1;

	TCHAR buf[512];
	_stprintf_s(buf, 512, _T("%d℃/%d℃"), lowT_, highT_);
	pL = new CTextLayer(this);
	pL->SetText(buf);
	pL->SetAnchor(0, 0);
	pL->SetFont(lg);
	pL->SetTextColor(RGB(80, 80, 80));
	sizePair = pL->GetTextSize();
	pL->SetPos(0, y);
	pL->SetSize(GetSize().first, (float)sizePair.cy);
	y += sizePair.cy + 5;

	CStaticImageNode *pPic = new CStaticImageNode(PresentCenter, this);
	CImageLayer* pImage=new CImageLayer(pPic);
	if(pImage->CreateImageLayerByFile(sPic_))
	{
		pPic->SetAnchor(0, 0);
		pPic->SetPos(0, y);
		pPic->SetSize(GetSize().first, (float)pImage->GetImageInfo().biHeight);
		pPic->SetImageLayer(pImage);
	}

	return CNode::Init();
	return true;
}