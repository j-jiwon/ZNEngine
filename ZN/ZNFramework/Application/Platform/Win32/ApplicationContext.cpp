#pragma once
#ifdef _WIN32
#include <Windows.h>
#include "ApplicationContext.h"
#include "../../../Graphics/ZNTexture.h"
#include "../../../Graphics/Platform/GraphicsAPI.h"
#include "../../../../ZNFramework.h"
#include "../../../Graphics/Platform/Direct3D12/CommandQueue.h"
#include "../../../Graphics/Platform/Direct3D12/CommandList.h"
#include "../../../Graphics/Platform/Direct3D12/SwapChain.h"

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
            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                Update();
                Render();
            }
        }
    }   

    return static_cast<int>(msg.wParam);
}

void ApplicationContext::Initialize(ZNWindow* inWindow, ZNGraphicsDevice* inDevice)
{
    if (inWindow == nullptr || inDevice == nullptr)
    {
        return;
    }    

    device = inDevice;
    commandQueue = device->CreateCommandQueue();
    swapChain = commandQueue->CreateSwapChain(inWindow);
    commandQueue->Init();
    //swapChain->Init();
    
    void* handler = static_cast<void*>(swapChain);
    auto HandlerResizeEvent = [=](size_t width, size_t height) {
        OnResize(width, height);
    };
    inWindow->AddEventHandler(handler, HandlerResizeEvent);

    OnResize(inWindow->Width(), inWindow->Height());
}

void ApplicationContext::OnResize(size_t width, size_t height)
{
    swapChain->Resize(width, height);
    commandQueue->OnResize(width, height);
}

void ApplicationContext::Update()
{

}

void ApplicationContext::Render()
{
    RenderBegin();
    // TODO 나머지 물체들 그려준다
    RenderEnd();
}

void ApplicationContext::RenderBegin()
{
    commandQueue->RenderBegin();
}

void ApplicationContext::RenderEnd()
{
    commandQueue->RenderEnd();
}

#endif