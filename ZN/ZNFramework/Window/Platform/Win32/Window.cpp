#pragma once
#include "Window.h"

using namespace ZNFramework;

Window::Window()
    :hwnd(nullptr)
    ,width(0)
    ,height(0)
{
}

void Window::Create()
{
    // Register the window class
    WNDCLASSEXW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = ::GetModuleHandleA(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.cbSize = sizeof(wc);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    // Create window
    hwnd = CreateWindowExW(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"This is ZNEngine Window",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        ::GetModuleHandle(NULL),  // Instance handle
        this        // Additional application data
    );

    if (hwnd == NULL)
    {
        throw std::exception("hwnd is null");
    }

    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void Window::Destroy()
{
    if (hwnd)
    {
        ::PostMessage(hwnd, WM_CLOSE, 0, 0);
        hwnd = nullptr;
    }
}

void Window::Show()
{
    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);
    }
}

void Window::Hide()
{
    if (hwnd)
    {
        ShowWindow(hwnd, SW_HIDE);
    }
}

LRESULT Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* window = (Window*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (window)
    {
        switch (uMsg)
        {
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                window->isPaused = true;
                window->timer.Stop();
            }
            else
            {
                window->isPaused = false;
                window->timer.Start();
            }
            return 0;
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
            uint32 width = LOWORD(lParam);
            uint32 height = HIWORD(lParam);

            if (width != window->width || height != window->height)
            {
                // broadcast.
                for (const auto& [key, value] : window->handlers)
                {
                    value(window->width, window->height);
                }
                window->width = width;
                window->height = height;
            }
        }
        return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
