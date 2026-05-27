
#include "../Common/DefineHeaders.h"
#include "ChatServerApp.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    // _CrtSetBreakAlloc(315);
    // _CrtSetBreakAlloc(306);

    
    if (!CChatServerApp::GetInstance()->Init(hInstance))
    {
        CChatServerApp::DestroyInstance();
        return 1;
    }

    CChatServerApp::GetInstance()->Run();

    CChatServerApp::DestroyInstance();

    return 0;
}