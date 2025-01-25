#include <windows.h>


HINSTANCE hInst;
char testtext[] = "Testestest";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

	HWND hwnd;
	MessageBox(NULL, testtext, testtext, MB_OK);
	return 0;
}