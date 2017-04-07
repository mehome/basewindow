#include <Windows.h>
#include <tchar.h>
#include "Mp3Player.h"
#include "MovieWindow.h"

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE,
	LPTSTR lpCmdLine,
	int nCmdShow)
{
	CApplication app;

	av_register_all();
	CMovieWindow movieWin;
	movieWin.InitWindow(hInstance);
	movieWin.Show();
	return movieWin.Run();

	CoInitialize(NULL);
	CMp3PlayerWindow win;
	win.InitWindow(hInstance);
	win.Show();
	auto res = win.Run();
	CoUninitialize();
	return res;
}
