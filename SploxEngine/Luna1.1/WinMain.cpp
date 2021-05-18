#include "Win64App.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	DX12GraphicsEngine rendering_engine;
	Win64App test(&rendering_engine, 1920, 1080, hInstance, nCmdShow);
	return test.Run();
}