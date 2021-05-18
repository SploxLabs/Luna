#include <Windows.h>
#include "DX12GraphicsEngine.h"
#include "DSAudioEngine.h"

class Win64App
{
public:
	Win64App(DX12GraphicsEngine* Engine, UINT Height, UINT Width, HINSTANCE hInstance, int nCmdShow);
	static int Run();
	static HWND GetHwnd() { return window_handle; }
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	static HWND window_handle;
	static DX12GraphicsEngine* graphics_engine;
	static DSAudioEngine audio_engine;

};

