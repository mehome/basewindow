#include <Windows.h>
#include <tchar.h>
#include "Thread.h"
#include "BaseWindow.h"
#include "BaseWebBroswer.h"


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	CoInitialize(NULL);
	CApplication app;
	//CTestSubControl* p=new CTestSubControl();
	//p->InitWindow(hInstance);
	//p->ReSize(600, 400);
	//p->RunWindow();
	CWebBrowserWindow::CheckRegister_FeatureBrowserEmulation();

	//CWebBrowserWindow win(L"C:\\Users\\Administrator.SKY-20150328LBV\\Desktop\\test.html");
	CWebBrowserWindow win(L"http://47.93.82.163/hp");
	//CWebBrowserWindow win(L"e:\\test.html");

	win.InitWindow(hInstance);
	win.ReSize(1280,720,true);
	win.Show();
	app.Run();

	CoUninitialize();
	return 0;
}