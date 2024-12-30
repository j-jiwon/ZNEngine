#pragma once
#ifdef _WIN32
#include <Windows.h>
#include "ApplicationContext.h"
#include "ZNFramework.h"
#include "ZNFramework/Graphics/Platform/GraphicsAPI.h"

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

    GraphicsContext::GetInstance().SetDevice(inDevice);
    WindowContext::GetInstance().SetWindow(inWindow);

    device = inDevice;
    commandQueue = ZNFramework::Platform::CreateCommandQueue();
    GraphicsContext::GetInstance().SetCommandQueue(commandQueue);

    swapChain = ZNFramework::Platform::CreateSwapChain();
    rootSignature = ZNFramework::Platform::CreateRootSignature();
    GraphicsContext::GetInstance().SetRootSignature(rootSignature);

    defaultShader = ZNFramework::Platform::CreateShader();
    defaultMesh = ZNFramework::Platform::CreateMesh();

    constantBuffer = ZNFramework::Platform::CreateConstantBuffer();
    GraphicsContext::GetInstance().SetConstantBuffer(constantBuffer);
    
    tableDescriptorHeap = ZNFramework::Platform::CreateTableDescriptorHeap();
    GraphicsContext::GetInstance().SetTableDescriptorHeap(tableDescriptorHeap);

    // initialize
    commandQueue->Init(swapChain);
    swapChain->Init(commandQueue);
    rootSignature->Init();
    constantBuffer->Init(sizeof(Transform), 256);
    tableDescriptorHeap->Init(256);
    
    // resize
    void* handler = static_cast<void*>(swapChain);
    auto HandlerResizeEvent = [=](size_t width, size_t height) {
        OnResize(width, height);
    };
    inWindow->AddEventHandler(handler, HandlerResizeEvent);

    OnResize(inWindow->Width(), inWindow->Height());

    // TEST 
    {
        std::filesystem::path shaderPath = GetExecutablePath().parent_path().parent_path() / L"Shaders" / L"default.hlsli";
        defaultShader->Load(shaderPath);

        std::vector<Vertex> vertices = {
            {ZNVector3(0.f, 0.5f, 0.5f), ZNVector4(1.f, 0.f, 0.f, 1.f)},
            {ZNVector3(0.5f, -0.5f, 0.5f), ZNVector4(0.f, 1.0f, 0.f, 1.f)},
            {ZNVector3(-0.5f, -0.5f, 0.5f), ZNVector4(0.f, 0.f, 1.f, 1.f)}
        };
        std::vector<uint32> indexVec;
        {
            indexVec.push_back(0);
            indexVec.push_back(1);
            indexVec.push_back(2);
        }
        {
            indexVec.push_back(0);
            indexVec.push_back(2);
            indexVec.push_back(3);
        }
        defaultMesh->Init(vertices, indexVec);
        defaultMesh->Render();
    }
}

void ApplicationContext::OnResize(size_t width, size_t height)
{
    swapChain->Resize(width, height);
}

void ApplicationContext::Update()
{
}

void ApplicationContext::Render()
{
    RenderBegin();

    defaultShader->Bind();
    {
        Transform t;
        t.offset = ZNVector4(0.0f, 0.f, 0.f, 0.f);
        defaultMesh->SetTransform(t);

        defaultMesh->Render();
    }

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