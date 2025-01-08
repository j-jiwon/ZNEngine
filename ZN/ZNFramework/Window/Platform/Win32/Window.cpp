#pragma once
#include "Window.h"
#include "Math/ZNMath.h"
#include "ZNInputDef.h"
#include <iostream>
#include <windowsx.h>
using namespace ZNFramework;

Window::Window()
    :hwnd(nullptr)
    ,width(700)
    ,height(500)
{
}

void Window::Create(uint32 inWidth, uint32 inHeight)
{
    // Register the window class
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    RECT R = { 0, 0, width, height };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int _width = R.right - R.left;
    int _height = R.bottom - R.top;

    // Create window
    hwnd = CreateWindowExW(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"This is ZNEngine Window",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, _width, _height,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        this        // Additional application data
    );

    if (hwnd == NULL)
    {
        throw std::exception("hwnd is null");
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
    WindowContext::GetInstance().SetWindow(this);
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

void Window::OnMouseEvent(MouseEvent mouseEvent)
{
    for (const auto& [key, value] : this->mouseEventHandlers)
    {
        value(mouseEvent);
    }
}

void Window::OnMouseMove(MouseEvent event)
{
    if (event.type != MOUSE_TYPE::RBUTTON)
    {
        float deltax = ConvertDegreesToRadians(0.25f * static_cast<float>(mousePos.x - event.x));
        float deltay = ConvertRadiansToDegrees(0.25f * static_cast<float>(mousePos.y - event.y));

        deltaPos.x = deltax;
        deltaPos.y = deltay;
    }
    mousePos.x = event.x;
    mousePos.y = event.y;
}

void Window::OnKeyboardEvent(KeyboardEvent event)
{
    for (const auto& [key, value] : this->keyboardEventHandlers)
    {
        value(event);
    }
}

LRESULT Window::MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* window = (Window*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
    //Window* window = WindowContext::GetInstance().GetAs<Window>();
    if (window == nullptr)
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return window->WindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT Window::WindowProc(HWND inHwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MouseEvent mouseEvent;
    MOUSE_TYPE mouseType = GetMouseTypeFromMsg(uMsg);
    KeyboardEvent keyboardEvent;
    KEY_TYPE keyType = ConverWindowsKeyToKeyType(wParam);

    switch (uMsg)
    {
    case WM_ACTIVATE:
    {
        if (LOWORD(wParam) == WA_INACTIVE) {
            isPaused = true;
            timer.Stop();
        }
        else {
            isPaused = false;
            timer.Start();
        }
        break;
    }
    case WM_SIZE:
    {
        uint32 _width = LOWORD(lParam);
        uint32 _height = HIWORD(lParam);

        if (_width != width || _height != height) {
            for (const auto& [key, value] : resizeEventHandlers) {
                value(_width, _height);
            }
            width = _width;
            height = _height;
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    // mouse
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        ::SetCapture(hwnd);
        mouseEvent.state = MOUSE_STATE::DOWN;
        mouseEvent.type = mouseType;
        mouseEvent.x = LOWORD(lParam);
        mouseEvent.y = HIWORD(lParam);
        OnMouseEvent(mouseEvent);
        break;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        ::ReleaseCapture();
        mouseEvent.state = MOUSE_STATE::UP;
        mouseEvent.type = mouseType;
        mouseEvent.x = LOWORD(lParam);
        mouseEvent.y = HIWORD(lParam);
        OnMouseEvent(mouseEvent);
        break;
    case WM_MOUSEMOVE:
        mouseEvent.state = MOUSE_STATE::UP;
        mouseEvent.type = mouseType;
        mouseEvent.x = LOWORD(lParam);
        mouseEvent.y = HIWORD(lParam);
        OnMouseMove(mouseEvent);
        OnMouseEvent(mouseEvent);
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        keyboardEvent.type = keyType;
        keyboardEvent.state = KEY_STATE::DOWN;
        OnKeyboardEvent(keyboardEvent);
        return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        keyboardEvent.type = keyType;
        keyboardEvent.state = KEY_STATE::UP;
        return 0;
    default:
        break;
    }
    return DefWindowProc(inHwnd, uMsg, wParam, lParam);
}

KEY_TYPE Window::ConverWindowsKeyToKeyType(WPARAM wParam)
{
    switch (wParam)
    {
    // Arrow Keys
    case VK_LEFT: return KEY_TYPE::KEY_LEFT;
    case VK_UP: return KEY_TYPE::KEY_UP;
    case VK_RIGHT: return KEY_TYPE::KEY_RIGHT;
    case VK_DOWN: return KEY_TYPE::KEY_DOWN;
        // Function Keys
    case VK_F1: return KEY_TYPE::KEY_F1;
    case VK_F2: return KEY_TYPE::KEY_F2;
    case VK_F3: return KEY_TYPE::KEY_F3;
    case VK_F4: return KEY_TYPE::KEY_F4;
    case VK_F5: return KEY_TYPE::KEY_F5;
    case VK_F6: return KEY_TYPE::KEY_F6;
    case VK_F7: return KEY_TYPE::KEY_F7;
    case VK_F8: return KEY_TYPE::KEY_F8;
    case VK_F9: return KEY_TYPE::KEY_F9;
    case VK_F10: return KEY_TYPE::KEY_F10;
    case VK_F11: return KEY_TYPE::KEY_F11;
    case VK_F12: return KEY_TYPE::KEY_F12;
        // Control Keys
    case VK_TAB: return KEY_TYPE::KEY_TAB;
    case VK_SHIFT: return KEY_TYPE::KEY_SHIFT;
    case VK_CONTROL: return KEY_TYPE::KEY_CONTROL;
    case VK_MENU: return KEY_TYPE::KEY_ALT;
    case VK_CAPITAL: return KEY_TYPE::KEY_CAPSLOCK;
    case VK_ESCAPE: return KEY_TYPE::KEY_ESC;
        // Navigation Keys
    case VK_PRIOR: return KEY_TYPE::KEY_PAGEUP;
    case VK_NEXT: return KEY_TYPE::KEY_PAGEDOWN;
    case VK_END: return KEY_TYPE::KEY_END;
    case VK_HOME: return KEY_TYPE::KEY_HOME;
        // Insert/Delete Keys
    case VK_INSERT: return KEY_TYPE::KEY_INSERT;
    case VK_DELETE: return KEY_TYPE::KEY_DELETE;
    case VK_BACK: return KEY_TYPE::KEY_BACKSPACE;
        // Numeric Keys
    case '0': return KEY_TYPE::KEY_0;
    case '1': return KEY_TYPE::KEY_1;
    case '2': return KEY_TYPE::KEY_2;
    case '3': return KEY_TYPE::KEY_3;
    case '4': return KEY_TYPE::KEY_4;
    case '5': return KEY_TYPE::KEY_5;
    case '6': return KEY_TYPE::KEY_6;
    case '7': return KEY_TYPE::KEY_7;
    case '8': return KEY_TYPE::KEY_8;
    case '9': return KEY_TYPE::KEY_9;
        // Alphanumeric Keys
    case 'A': return KEY_TYPE::KEY_A;
    case 'B': return KEY_TYPE::KEY_B;
    case 'C': return KEY_TYPE::KEY_C;
    case 'D': return KEY_TYPE::KEY_D;
    case 'E': return KEY_TYPE::KEY_E;
    case 'F': return KEY_TYPE::KEY_F;
    case 'G': return KEY_TYPE::KEY_G;
    case 'H': return KEY_TYPE::KEY_H;
    case 'I': return KEY_TYPE::KEY_I;
    case 'J': return KEY_TYPE::KEY_J;
    case 'K': return KEY_TYPE::KEY_K;
    case 'L': return KEY_TYPE::KEY_L;
    case 'M': return KEY_TYPE::KEY_M;
    case 'N': return KEY_TYPE::KEY_N;
    case 'O': return KEY_TYPE::KEY_O;
    case 'P': return KEY_TYPE::KEY_P;
    case 'Q': return KEY_TYPE::KEY_Q;
    case 'R': return KEY_TYPE::KEY_R;
    case 'S': return KEY_TYPE::KEY_S;
    case 'T': return KEY_TYPE::KEY_T;
    case 'U': return KEY_TYPE::KEY_U;
    case 'V': return KEY_TYPE::KEY_V;
    case 'W': return KEY_TYPE::KEY_W;
    case 'X': return KEY_TYPE::KEY_X;
    case 'Y': return KEY_TYPE::KEY_Y;
    case 'Z': return KEY_TYPE::KEY_Z;
        // Miscellaneous Keys
    case VK_RETURN: return KEY_TYPE::KEY_ENTER;
    case VK_SPACE: return KEY_TYPE::KEY_SPACE;
    case VK_OEM_1: return KEY_TYPE::KEY_SEMICOLON;
    case VK_OEM_PLUS: return KEY_TYPE::KEY_EQUAL;
    case VK_OEM_COMMA: return KEY_TYPE::KEY_COMMA;
    case VK_OEM_MINUS: return KEY_TYPE::KEY_MINUS;
    case VK_OEM_PERIOD: return KEY_TYPE::KEY_PERIOD;
    case VK_OEM_2: return KEY_TYPE::KEY_SLASH;
    case VK_OEM_5: return KEY_TYPE::KEY_BACKSLASH;
    case VK_OEM_4: return KEY_TYPE::KEY_LEFTBRACKET;
    case VK_OEM_6: return KEY_TYPE::KEY_RIGHTBRACKET;
    case VK_OEM_7: return KEY_TYPE::KEY_APOSTROPHE;
        // Default case
    default: return KEY_TYPE::KEY_UNKNOWN;
    }
}

MOUSE_TYPE Window::GetMouseTypeFromMsg(UINT uMsg)
{
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        return MOUSE_TYPE::LBUTTON;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        return MOUSE_TYPE::RBUTTON;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        return MOUSE_TYPE::MBUTTON;
    default:
        return MOUSE_TYPE::UNKNOWN;
    }
}
