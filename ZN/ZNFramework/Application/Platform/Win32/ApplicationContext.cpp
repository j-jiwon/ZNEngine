#ifdef _WIN32
#include <Windows.h>
#include <iostream>
#include "ApplicationContext.h"
#include "ZNFramework.h"
#include "ZNFramework/Graphics/Platform/GraphicsAPI.h"
#include "ZNFramework/Scene/ZNScene.h"
#include "ZNFramework/ZNCamera.h"

using namespace ZNFramework;
using namespace std;

int ApplicationContext::MessageLoop()
{
    MSG msg = {};
    timer = new ZNTimer();
    timer->Reset();

    // Main message loop
    while (true)
    {
        // Process all pending messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // No messages to process, update and render
            timer->Tick();
            Update();
            Render();
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

    constantBuffer = ZNFramework::Platform::CreateConstantBuffer();
    GraphicsContext::GetInstance().SetConstantBuffer(constantBuffer);
    
    tableDescriptorHeap = ZNFramework::Platform::CreateTableDescriptorHeap();
    GraphicsContext::GetInstance().SetTableDescriptorHeap(tableDescriptorHeap);

    depthStencilBuffer = ZNFramework::Platform::CreateDepthStencilBuffer();
    GraphicsContext::GetInstance().SetDepthStencilBuffer(depthStencilBuffer);

    // Initialize graphics resources
    commandQueue->Init(swapChain);
    swapChain->Init(commandQueue);
    rootSignature->Init();
    constantBuffer->Init(sizeof(TransformMatrices), 256); // Use TransformMatrices size for MVP
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

    // Load default shader
    {
        std::filesystem::path shaderPath = GetResourcePath() / L"Shaders" / L"default.hlsli";
        defaultShader->Load(shaderPath);
    }

    commandQueue->WaitSync();
}

void ApplicationContext::SetScene(ZNScene* scene)
{
    currentScene = scene;
}

ZNScene* ApplicationContext::GetScene() const
{
    return currentScene;
}

void ApplicationContext::OnResize(uint32 width, uint32 height)
{
    swapChain->Resize(width, height);
    depthStencilBuffer->Init();

    // Update camera projection from scene
    if (currentScene && currentScene->GetCamera())
    {
        ZNCamera* camera = currentScene->GetCamera();
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        camera->SetPerspective(3.141592f / 4.0f, aspect, 0.1f, 100.0f); // 45 degrees FOV
        std::cout << "Resize: width=" << width << ", height=" << height << ", aspect=" << aspect << std::endl;
    }
}

void ApplicationContext::OnMouseEvent(struct MouseEvent event)
{
    if (currentScene && currentScene->GetCamera())
    {
        currentScene->GetCamera()->ProcessMouse(event);
    }
}

void ApplicationContext::OnKeyboardEvent(struct KeyboardEvent event)
{
    if (currentScene && currentScene->GetCamera())
    {
        static int callCount = 0;
        if (callCount++ < 5)
        {
            std::cout << "Key Event: type=" << static_cast<int>(event.type)
                      << ", state=" << static_cast<int>(event.state)
                      << ", deltaTime=" << timer->DeltaTime() << std::endl;
        }
        currentScene->GetCamera()->ProcessKeyboard(event, timer->DeltaTime());
    }
}

void ApplicationContext::Update()
{
    if (currentScene)
    {
        currentScene->Update(timer->DeltaTime());
    }
}

void ApplicationContext::Render()
{
    RenderBegin();

    if (currentScene)
    {
        currentScene->Render();
    }

    RenderEnd();
}

void ApplicationContext::RenderBegin()
{
    // CommandQueue handles all render target setup
    commandQueue->RenderBegin();
}

void ApplicationContext::RenderEnd()
{
    commandQueue->RenderEnd();
}

#endif