#ifndef UNICODE
#define UNICODE
#endif 

#include <exception>
#include "ZNWindow.h"
#include "../ZNInclude.h"

using namespace ZNFramework;

ZNWindow::ZNWindow()
	:hwnd(nullptr)
    ,width(0)
    ,height(0)
{
}

ZNWindow::~ZNWindow()
{
}

void ZNWindow::Create()
{
    // Register the window class
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = ::GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Create window
    hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"This is ZNEngine Window",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        ::GetModuleHandle(NULL),  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL)
    {
        throw std::exception("hwnd is null");
    }

    ShowWindow(hwnd, 1);
}

void ZNWindow::Destroy()
{
    if (hwnd)
    {
        ::PostMessage(hwnd, WM_CLOSE, 0, 0);
        hwnd = nullptr;
    }
}

void ZNWindow::AddEventHandler(EventHandler handler, ResizeEventCallback callback)
{
    handlers.emplace(handler, callback);
}

void ZNWindow::RemoveEventHandler(EventHandler handler)
{
    handlers.erase(handler);
}

LRESULT ZNWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ZNWindow* window = (ZNWindow*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (window)
    {
        switch (uMsg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            EndPaint(hwnd, &ps);
        }
        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            if (width != window->width || height != window->height)
            {
                // broadcast.
                for (const auto& [key, value] : window->handlers)
                {
                    value(window->width, window->height);
                }
            }
        }
        return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
