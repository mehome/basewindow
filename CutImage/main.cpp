#include <Windows.h>
#include <tchar.h>
#include "CutImage.h"
#include "Thread.h"
#include <functional>

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

	CTask0<CApplication> task0(&app, &CApplication::Destroy);
	task0.Do();
	return 0;

	CThread thread1;
	thread1.Init();
	for (int i = 0; i < 10000; ++i)
	{
		PostThreadMessage(thread1.ThreadId(), WM_USER +10, 0, 0);
		Sleep(1);
	}
	thread1.Destroy();

	CCutImageWindow win;
	win.InitWindow(hInstance);
	win.Show();

	return app.Run();
}