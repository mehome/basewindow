#pragma once
#include <Node.h>
#include "HttpBase.h"
class WeatherScene : public CScene
{
public:
	bool Init();
protected:
	std::wstring GetCity();
	bool GetWeather(const std::wstring& cityName);
protected:
	std::wstring m_szCityName;
	JsonValue m_jsonWeather;
	JsonValue m_jsonPM25;
};

class WeatherDayGeneral: public CNode
{
public:
	WeatherDayGeneral(const std::wstring& date, int low, int high, const std::wstring& pic, CNode* pPaent=NULL);
	bool Init();
protected:
	std::wstring sDate_;
	int lowT_;
	int highT_;
	std::wstring sPic_;
};