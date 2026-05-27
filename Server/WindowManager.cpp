#include "WindowManager.h"
#include "imgui_impl_win32.h"
#include "Resource.h"

UINT CWindowManager::mResizeWidth = 0;
UINT CWindowManager::mResizeHeight = 0;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


LRESULT WINAPI CWindowManager::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        mResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        mResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool CWindowManager::Init(HINSTANCE _hInstance)
{
    LoadStringW(_hInstance, IDS_APP_TITLE, mTitleName, MAX_LOADSTRING);
    LoadStringW(_hInstance, IDC_SERVER, mClassName, MAX_LOADSTRING);

    mWC = { sizeof(mWC), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ATOM atom = RegisterClassExW(&mWC);

    if (atom == 0)
        return false;

    ImGui_ImplWin32_EnableDpiAwareness();
    mMainScale = ImGui_ImplWin32_GetDpiScaleForMonitor
        (::MonitorFromWindow(mhWnd, MONITOR_DEFAULTTONEAREST));

    mhWnd = ::CreateWindowW(mWC.lpszClassName, L"Chat Server App", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 100, 100, (int)300 * mMainScale, (int)400 * mMainScale, nullptr, nullptr, mWC.hInstance, nullptr);

    if (nullptr == mhWnd)
        return false;

    return true;
}

bool CWindowManager::MessageLoop()
{
    bool done = false;
    
    MSG msg;
    
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
            done = true;
    }

    return done;
}

void CWindowManager::Shutdown()
{
    ::DestroyWindow(mhWnd);
    UnRegisterWindowClass();
}

void CWindowManager::UnRegisterWindowClass()
{
    ::UnregisterClassW(mWC.lpszClassName, mWC.hInstance);
}

void CWindowManager::ShowWindow()
{
    ::ShowWindow(mhWnd, SW_SHOWDEFAULT);
    ::UpdateWindow(mhWnd);
}

void CWindowManager::ClearResizeResoultion()
{
    mResizeWidth = mResizeHeight = 0;
}
