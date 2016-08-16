#pragma once
#include <Windows.h>
#include <string>
#include "Log.h"

class CBaseWindow
{
	friend LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
	CBaseWindow();
	~CBaseWindow();

	HWND GetHWND();
	void ReSize(int w, int h,bool bCenterWindow=false);
	virtual void InitWindow(HINSTANCE hInstance, HWND hWnd=NULL, bool bAsChildOrOwned=true);
	virtual void Show();
	virtual LRESULT CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed);
protected:
	HWND m_hWnd;
};

class CTestSubControl : public CBaseWindow
{
public:
	LRESULT CustomProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool& bProcessed);
};