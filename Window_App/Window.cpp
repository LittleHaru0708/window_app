#include <Windows.h>
#include "DX12App.h"

DX12App* g_app = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        // ウィンドウサイズ変更時に何かするならここ
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    const char CLASS_NAME[] = "DX12WindowClass";

    WNDCLASS wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClass(&wc))
        return -1;

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "DirectX12 Triangle Demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd) return -1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // DX12 初期化
    g_app = new DX12App(hwnd, 1280, 720);
    if (!g_app->Initialize())
    {
        MessageBoxA(hwnd, "DirectX12 initialization failed.", "Error", MB_OK | MB_ICONERROR);
        delete g_app;
        g_app = nullptr;
        return -1;
    }

    // メインループ
    MSG msg{};
    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // 描画
            g_app->Render();
        }
    }

    delete g_app;
    g_app = nullptr;

    return static_cast<int>(msg.wParam);
}
