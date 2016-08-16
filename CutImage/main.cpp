#include <Windows.h>
#include <tchar.h>
#include "CutImage.h"
#include "Thread.h"

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE,
                     LPTSTR lpCmdLine,
                     int nCmdShow)
{
	CApplication app;

	//CTestSubControl* p=new CTestSubControl();
	//p->InitWindow(hInstance, NULL);
	//p->ReSize(600, 400, true);
	//p->Show();

	CCutImageWindow win;
	win.InitWindow(hInstance);
	win.Show();

	return app.Run();
}