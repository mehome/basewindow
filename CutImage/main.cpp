#include <Windows.h>
#include <tchar.h>
#include "CutImage.h"

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	//CTestSubControl* p=new CTestSubControl();
	//p->InitWindow(hInstance, NULL);
	//p->ReSize(600, 400, true);
	//p->RunWindow();

	CCutImageWindow win;
	win.InitWindow(hInstance);
	win.RunWindow();

	return 0;
}