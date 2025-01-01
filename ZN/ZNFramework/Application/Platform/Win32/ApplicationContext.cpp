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
    defaultTexture = ZNFramework::Platform::CreateTexture();

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
    auto HandlerResizeEvent = [=](uint32 width, uint32 height) {
        OnResize(width, height);
    };
    inWindow->AddEventHandler(handler, HandlerResizeEvent);

    OnResize(inWindow->Width(), inWindow->Height());

    // TEST 
    {
        std::vector<Vertex> vertices(4);
        vertices[0].pos = ZNVector3(-0.5f, 0.5f, 0.5f);
        vertices[0].color = ZNVector4(1.f, 0.f, 0.f, 1.f);
        vertices[0].uv = ZNVector2(0.f, 0.f);
        vertices[1].pos = ZNVector3(0.5f, 0.5f, 0.5f);
        vertices[1].color = ZNVector4(0.f, 1.f, 0.f, 1.f);
        vertices[1].uv = ZNVector2(1.f, 0.f);
        vertices[2].pos = ZNVector3(0.5f, -0.5f, 0.5f);
        vertices[2].color = ZNVector4(0.f, 0.f, 1.f, 1.f);
        vertices[2].uv = ZNVector2(1.f, 1.f);
        vertices[3].pos = ZNVector3(-0.5f, -0.5f, 0.5f);
        vertices[3].color = ZNVector4(0.f, 1.f, 0.f, 1.f);
        vertices[3].uv = ZNVector2(0.f, 1.f);

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

        std::filesystem::path shaderPath = GetResourcePath() / L"Shaders" / L"default.hlsli";
        defaultShader->Load(shaderPath);

        defaultMesh->Init(vertices, indexVec);

        std::filesystem::path texturePath = GetResourcePath() / L"Textures" / L"lutz.png";
        defaultTexture->Init(texturePath);

        defaultMesh->SetTexture(defaultTexture);
        defaultMesh->Render();
    }
}

void ApplicationContext::OnResize(uint32 width, uint32 height)
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