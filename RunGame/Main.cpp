#include "RunGame.h"

//��ں���
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	RunGame theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}