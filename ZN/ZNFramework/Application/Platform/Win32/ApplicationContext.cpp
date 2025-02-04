#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <iostream>
#include "ApplicationContext.h"
#include "ZNFramework.h"
#include "ZNFramework/Graphics/Platform/GraphicsAPI.h"

using namespace ZNFramework;
using namespace std;

int ApplicationContext::MessageLoop()
{
    MSG msg;
    timer = new ZNTimer();
    timer->Reset();
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
                timer->Tick();
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
    //WindowContext::GetInstance().SetWindow(inWindow);

    device = inDevice;
    commandQueue = ZNFramework::Platform::CreateCommandQueue();
    GraphicsContext::GetInstance().SetCommandQueue(commandQueue);

    swapChain = ZNFramework::Platform::CreateSwapChain();
    rootSignature = ZNFramework::Platform::CreateRootSignature();
    GraphicsContext::GetInstance().SetRootSignature(rootSignature);

    defaultShader = ZNFramework::Platform::CreateShader();
    defaultMesh = ZNFramework::Platform::CreateMesh();
    defaultTexture = ZNFramework::Platform::CreateTexture();

    constantBuffer = ZNFramework::Platform::CreateConstantBuffer();
    GraphicsContext::GetInstance().SetConstantBuffer(constantBuffer);
    
    tableDescriptorHeap = ZNFramework::Platform::CreateTableDescriptorHeap();
    GraphicsContext::GetInstance().SetTableDescriptorHeap(tableDescriptorHeap);

    depthStencilBuffer = ZNFramework::Platform::CreateDepthStencilBuffer();
    GraphicsContext::GetInstance().SetDepthStencilBuffer(depthStencilBuffer);

    // initialize
    commandQueue->Init(swapChain);
    swapChain->Init(commandQueue);
    rootSignature->Init();
    constantBuffer->Init(sizeof(Transform), 256);
    tableDescriptorHeap->Init(256);
    depthStencilBuffer->Init();
    
    // resize
    void* handler = static_cast<void*>(swapChain);
    auto HandlerResizeEvent = [=](uint32 width, uint32 height) {
        OnResize(width, height);
    };
    auto HandlerMouseEvent = [=](MouseEvent event) {
        OnMouseEvent(event);
    };
    auto HandlerKeyboardEvent = [=](KeyboardEvent event) {
        OnKeyboardEvent(event);
    };
    inWindow->AddEventHandler(handler, HandlerResizeEvent, HandlerMouseEvent, HandlerKeyboardEvent);

    OnResize(inWindow->Width(), inWindow->Height());

    // TEST 
    {
        std::vector<Vertex> vec(4);
        vec[0].pos = ZNVector3(-0.5f, 0.5f, 0.f);
        vec[0].color = ZNVector4(1.f, 0.f, 0.f, 1.f);
        vec[0].uv = ZNVector2(0.f, 0.f);
        vec[1].pos = ZNVector3(0.5f, 0.5f, 0.f);
        vec[1].color = ZNVector4(0.f, 1.f, 0.f, 1.f);
        vec[1].uv = ZNVector2(1.f, 0.f);
        vec[2].pos = ZNVector3(0.5f, -0.5f, 0.f);
        vec[2].color = ZNVector4(0.f, 0.f, 1.f, 1.f);
        vec[2].uv = ZNVector2(1.f, 1.f);
        vec[3].pos = ZNVector3(-0.5f, -0.5f, 0.f);
        vec[3].color = ZNVector4(0.f, 1.f, 0.f, 1.f);
        vec[3].uv = ZNVector2(0.f, 1.f);

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
        defaultMesh->Init(vec, indexVec);

        std::filesystem::path shaderPath = GetResourcePath() / L"Shaders" / L"default.hlsli";
        defaultShader->Load(shaderPath);

        std::filesystem::path texturePath = GetResourcePath() / L"Textures" / L"lutz.webp";
        defaultTexture->Init(texturePath);
    }
    commandQueue->WaitSync();
}

void ApplicationContext::OnResize(uint32 width, uint32 height)
{
    swapChain->Resize(width, height);
    depthStencilBuffer->Init();
}

void ApplicationContext::OnMouseEvent(MouseEvent event)
{
}

void ApplicationContext::OnKeyboardEvent(KeyboardEvent event)
{
    std::cout << static_cast<int>(event.type) << std::endl;
}

void ApplicationContext::Update()
{
}

void ApplicationContext::Render()
{
    RenderBegin();

    defaultShader->Bind();
    {
        Transform t1;
        t1.offset = ZNVector4(0.25f, 0.25f, 0.3f, 0.f);
        defaultMesh->SetTransform(t1);
        defaultMesh->SetTexture(defaultTexture);
        defaultMesh->Render();
    }
    {
        Transform t;
        t.offset = ZNVector4(0.0f, 0.f, 0.2f, 0.f);
        defaultMesh->SetTransform(t);
        defaultMesh->SetTexture(defaultTexture);
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