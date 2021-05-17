#include "Win64App.h"


HWND Win64App::window_handle = nullptr;
DX12GraphicsEngine* Win64App::graphics_engine = nullptr;
DSAudioEngine Win64App::audio_engine = DSAudioEngine();

Win64App::Win64App(DX12GraphicsEngine* Engine, UINT Height, UINT Width, HINSTANCE hInstance, int nCmdShow)
{
    
    graphics_engine = Engine;
    // Initialize the window class.
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"DXSampleClass";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    window_handle = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        windowClass.lpszClassName,
        L"window_name",
        WS_SYSMENU,
        0, 0,
        Width, Height,
        nullptr,        // We have no parent window.
        nullptr,        // We aren't using menus.
        hInstance,
        NULL);//might be bad

    graphics_engine->render_window = window_handle;
    graphics_engine->Init();

    audio_engine = DSAudioEngine(window_handle);
    ShowWindow(window_handle, nCmdShow);
}

int Win64App::Run()
{
    SetWindowText(window_handle, L"[REORIENT CRAFT-> (Roll(Q,E), Pitch(W,S), Yaw(A,D)),  SLOW DESCENT->(Thruster(SPACE) WARNING: MANAGE FUEL RESERVE (20s)] ");
    ShowWindow(window_handle, SW_SHOW);
    // Main sample loop.

    //start looping audio
    audio_engine.thruster->SetVolume(DSBVOLUME_MIN);
    audio_engine.thruster->Play(0, 0, DSBPLAY_LOOPING);

    audio_engine.rcs->SetVolume(DSBVOLUME_MIN);
    audio_engine.rcs->Play(0, 0, DSBPLAY_LOOPING);

    //audio_engine.alarm->SetVolume(DSBVOLUME_MAX)
    //audio_engine.alarm->Play(0, 0, DSBPLAY_LOOPING);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return static_cast<char>(msg.wParam);
}

LRESULT Win64App::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    //DX12GraphicsEngine* tmp_graphics_engine = reinterpret_cast<DX12GraphicsEngine*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (graphics_engine->camera_rb.fuel < 0) 
    {
        audio_engine.thruster->Stop();
    }

    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) 
        {
            PostQuitMessage(0);    
        }

        if (wParam == 0x57) { graphics_engine->camera_rb.roll_left = true; } //A
        if (wParam == 0x53) { graphics_engine->camera_rb.roll_right = true; } //Ddd
        if (wParam == 0x44) { graphics_engine->camera_rb.pitch_up = true; } //S
        if (wParam == 0x41) { graphics_engine->camera_rb.pitch_down = true; } //W
        if (wParam == 0x51) { graphics_engine->camera_rb.yaw_left = true; } //Q
        if (wParam == 0x45) { graphics_engine->camera_rb.yaw_right = true; } //E

        if (wParam == VK_SPACE) 
        { 
            graphics_engine->camera_rb.thurster_activation = true;
            if (graphics_engine->camera_rb.fuel > 0) 
            {
                audio_engine.thruster->SetVolume(DSBVOLUME_MAX);  
            }
        };

        if (graphics_engine->camera_rb.roll_left ||
            graphics_engine->camera_rb.roll_right ||
            graphics_engine->camera_rb.pitch_up ||
            graphics_engine->camera_rb.pitch_down ||
            graphics_engine->camera_rb.yaw_left ||
            graphics_engine->camera_rb.yaw_right) 
        {

            audio_engine.rcs->SetVolume(DSBVOLUME_MAX);
        }
        return 0;
    case WM_KEYUP:
        if (wParam == 0x57) { graphics_engine->camera_rb.roll_left = false; } //A
        if (wParam == 0x53) { graphics_engine->camera_rb.roll_right = false; } //D
        if (wParam == 0x44) { graphics_engine->camera_rb.pitch_up = false; } //S
        if (wParam == 0x41) { graphics_engine->camera_rb.pitch_down = false; } //W
        if (wParam == 0x51) { graphics_engine->camera_rb.yaw_left = false; } //Q
        if (wParam == 0x45) { graphics_engine->camera_rb.yaw_right = false; } //E
        if (!graphics_engine->camera_rb.roll_left &&
            !graphics_engine->camera_rb.roll_right &&
            !graphics_engine->camera_rb.pitch_up &&
            !graphics_engine->camera_rb.pitch_down &&
            !graphics_engine->camera_rb.yaw_left &&
            !graphics_engine->camera_rb.yaw_right) {
            audio_engine.rcs->SetVolume(DSBVOLUME_MIN);
        }
        if (wParam == VK_SPACE) {
            graphics_engine->camera_rb.thurster_activation = false;
            audio_engine.thruster->SetVolume(DSBVOLUME_MIN);
        };
        return 0;
    case WM_PAINT:
        if (graphics_engine) 
        {
            graphics_engine->Update();
            graphics_engine->Render();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}
