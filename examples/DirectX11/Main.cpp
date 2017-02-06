#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <iostream>

#define XRES    1366
#define YRES     768

//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
int main()
{
	// Create window
	HWND wnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 0, 0, XRES, YRES, 0, 0, 0, 0);
	HDC hdc = GetDC(wnd);


	// begin main loop
	bool running = true;
	while (running)
	{
		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) {
				std::cout << "WM_QUIT";
				running = false;
			}

			if (msg.message == WM_CLOSE) {
				std::cout << "WM_CLOSE";
				running = false;
			}

			if (msg.message == WM_DESTROY) {
				std::cout << "WM_DESTROY";
				PostQuitMessage(0);
				running = false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//glColor3us((unsigned short)GetTickCount(), 0, 0);
		//glRects(-1, -1, 1, 1);
		SwapBuffers(hdc);
	}

	return 0;
}