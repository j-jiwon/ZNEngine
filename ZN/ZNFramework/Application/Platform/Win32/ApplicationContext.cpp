#pragma once
#ifdef _WIN32
#include <Windows.h>
#include "ApplicationContext.h"
using namespace ZNFramework;

int ApplicationContext::MessageLoop()
{
    MSG msg;
    // loop while message is not WM_QUIT 
    // GetMessage returns Message.wParam when message loop is terminated.
    while (auto ret = GetMessage(&msg, NULL, 0, 0))
    {
        if (ret == -1)
        {
            // handle the error and possibly exit
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }   

    return static_cast<int>(msg.wParam);
}

#endif