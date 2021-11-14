#include "ZNApplication.h"
#include <windows.h>

using namespace ZNFramework;

ZNApplication::ZNApplication()
{
}

void ZNApplication::Run()
{   
    // process platform message
    MSG msg = {};
    while (GetMessage(&msg, (HWND)NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
