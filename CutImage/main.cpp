#include <Windows.h>
#include <tchar.h>
#include "CutImage.h"
#include "Mp3Player.h"
#include "Thread.h"
#include "Task.h"

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

	MSG msg;
	CTask1<CApplication, const MSG&, int> task1(&app, &CApplication::HandleQueueMessage, msg);
	task1.Do();

	//CThread thread1;
	//thread1.Init();
	//for (int i = 0; i < 10000; ++i)
	//{
	//	PostThreadMessage(thread1.ThreadId(), WM_USER +10, 0, 0);
	//	Sleep(1);
	//}
	//thread1.Destroy();

	//CCutImageWindow win;
	//win.InitWindow(hInstance);
	//win.Show();
	//return app.Run();

	CoInitialize(NULL);
	CMp3PlayerWindow win;
	win.InitWindow(hInstance);
	win.Show();
	auto res=win.Run();
	CoUninitialize();
	return res;
}