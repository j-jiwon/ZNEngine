#ifdef _WIN32
#include <Windows.h>
#include <iostream>
#include "ApplicationContext.h"
#include "ZNFramework.h"
#include "ZNFramework/UI/Platform/Win32_DX12/ImGuiLayer.h"
#include "imgui.h"
#include "ZNFramework/Graphics/Platform/GraphicsAPI.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/Shader.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/CommandQueue.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/GraphicsDevice.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/GBufferManager.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/DeferredLightingPass.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/DebugViewportRenderer.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/ShadowMap.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/DirectionalLight.h"
#include "ZNFramework/Scene/ZNScene.h"
#include "ZNFramework/Scene/ZNGameObject.h"
#include "ZNFramework/ZNCamera.h"
#include "ZNFramework/Math/ZNMatrix4.h"

using namespace ZNFramework;
using namespace ZNFramework::Platform::Direct3D;
using namespace std;

ApplicationContext::~ApplicationContext()
{
	if (imguiLayer)
	{
		imguiLayer->Shutdown();
		delete imguiLayer;
		imguiLayer = nullptr;
	}
}

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
    constantBuffer->Init(sizeof(TransformMatrices), 2048);
    tableDescriptorHeap->Init(2048);
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

    // Load shadow depth shader for shadow pass
    {
        shadowDepthShader = ZNFramework::Platform::CreateShader();
        std::filesystem::path shaderPath = GetResourcePath() / L"Shaders" / L"shadow_depth.hlsli";
        shadowDepthShader->Load(shaderPath);

        // Shadow pass has no render targets, only depth
        Shader* d3dShader = dynamic_cast<Shader*>(shadowDepthShader);
        if (d3dShader)
        {
            d3dShader->SetRenderTargetFormats(0, nullptr);
        }
    }

    // Load G-Buffer shader for MRT
    {
        gbufferShader = ZNFramework::Platform::CreateShader();
        std::filesystem::path shaderPath = GetResourcePath() / L"Shaders" / L"gbuffer.hlsli";
        gbufferShader->Load(shaderPath);

        // Configure for 3 render targets
        DXGI_FORMAT mrtFormats[3] = {
            DXGI_FORMAT_R8G8B8A8_UNORM,      // Base Color
            DXGI_FORMAT_R16G16B16A16_FLOAT,  // World Normal
            DXGI_FORMAT_R32_FLOAT            // Depth
        };

        // Cast to concrete type to access SetRenderTargetFormats
        Shader* d3dShader = dynamic_cast<Shader*>(gbufferShader);
        if (d3dShader)
        {
            d3dShader->SetRenderTargetFormats(3, mrtFormats);
        }

        GraphicsContext::GetInstance().SetGBufferShader(gbufferShader);
    }

    // Initialize G-Buffer and Debug Viewport Renderer after SwapChain is ready
    // This must be done after OnResize to ensure proper dimensions
    CommandQueue* cmdQueue = dynamic_cast<CommandQueue*>(commandQueue);
    if (cmdQueue)
    {
        // Initialize G-Buffer for MRT
        GBufferManager* gbufferMgr = new GBufferManager();
        gbufferMgr->Init(inWindow->Width(), inWindow->Height());
        cmdQueue->SetGBufferManager(gbufferMgr);

        // Initialize Deferred Lighting Pass
        DeferredLightingPass* lightingPass = new DeferredLightingPass();
        lightingPass->Init();
        cmdQueue->SetDeferredLightingPass(lightingPass);

        // Initialize Debug Viewport Renderer (disabled — replaced by ImGui GBuffer Preview)
        DebugViewportRenderer* debugViewport = new DebugViewportRenderer();
        debugViewport->Init();
        debugViewport->SetEnabled(false);
        cmdQueue->SetDebugViewportRenderer(debugViewport);

        // Initialize Shadow Map (2048x2048)
        ShadowMap* shadowMap = new ShadowMap();
        shadowMap->Init(2048, 2048);
        cmdQueue->SetShadowMap(shadowMap);
    }

    commandQueue->WaitSync();

    // ImGui 초기화
    {
        using namespace ZNFramework::Platform::Direct3D;
        ImGuiLayer* guiLayer = new ImGuiLayer();
        HWND hwnd = (HWND)inWindow->PlatformHandle();
        GraphicsDevice* gfxDevice = dynamic_cast<GraphicsDevice*>(device);
        CommandQueue* d3dCmdQueue = dynamic_cast<CommandQueue*>(commandQueue);
        guiLayer->Init(hwnd, gfxDevice->Device().Get(), d3dCmdQueue->Queue(), SWAP_CHAIN_BUFFER_COUNT, DXGI_FORMAT_R8G8B8A8_UNORM);
        imguiLayer = guiLayer;

        commandQueue->SetImGuiDescriptorHeap(guiLayer->GetSrvHeap());

        // Register GBuffer SRVs as ImGui textures for debug windows
        GBufferManager* gbufferMgr = d3dCmdQueue->GetGBufferManager();
        ShadowMap* shadowMapPtr    = d3dCmdQueue->GetShadowMap();
        DebugViewportRenderer* debugRenderer = d3dCmdQueue->GetDebugViewportRenderer();

        const bool hasShadow = (shadowMapPtr != nullptr);

        commandQueue->SetImGuiRenderCallback([this, guiLayer, gfxDevice, d3dCmdQueue, hasShadow]()
        {
            GBufferManager* gbufferMgr = d3dCmdQueue->GetGBufferManager();
            ShadowMap* shadowMapPtr    = d3dCmdQueue->GetShadowMap();

            // Refresh descriptors every frame so GBuffer resize is automatically reflected
            ImTextureID baseColorTexId = guiLayer->SetTexture(gfxDevice->Device().Get(), gbufferMgr->GetBaseColorSRV(), 1);
            ImTextureID normalTexId    = guiLayer->SetTexture(gfxDevice->Device().Get(), gbufferMgr->GetNormalSRV(), 2);
            ImTextureID worldPosTexId  = guiLayer->SetTexture(gfxDevice->Device().Get(), gbufferMgr->GetWorldPosSRV(), 3);
            ImTextureID armTexId       = guiLayer->SetTexture(gfxDevice->Device().Get(), gbufferMgr->GetARMSRV(), 4);
            ImTextureID depthTexId     = guiLayer->SetGrayscaleTexture(gfxDevice->Device().Get(), gbufferMgr->GetDepthCopyResource(), DXGI_FORMAT_R32_FLOAT, 5);
            ImTextureID shadowTexId    = (hasShadow && shadowMapPtr) ? guiLayer->SetGrayscaleTexture(gfxDevice->Device().Get(), shadowMapPtr->GetResource(), DXGI_FORMAT_R32_FLOAT, 6) : 0;

            ImVec2 thumbSize(160.0f, 90.0f);
            ImGui::SetNextWindowSize(ImVec2(200.0f, 0.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("GBuffer Preview");
            auto thumb = [&](const char* label, ImTextureID tex) {
                ImGui::Text("%s", label);
                ImGui::Image(tex, thumbSize);
            };
            thumb("BaseColor", baseColorTexId);
            thumb("Normal",    normalTexId);
            thumb("WorldPos",  worldPosTexId);
            thumb("ARM",       armTexId);
            thumb("Depth",     depthTexId);
            if (shadowTexId != 0)
                thumb("Shadow", shadowTexId);
            ImGui::End();

            imguiLayer->EndFrame();
        });
    }
}

void ApplicationContext::SetScene(ZNScene* scene)
{
    currentScene = scene;

    if (commandQueue && currentScene)
    {
        // Forward pass: scene UI + transparent objects
        commandQueue->SetForwardRenderCallback([this]() {
            if (currentScene) currentScene->RenderForward();
        });

        CommandQueue* cmdQueue = dynamic_cast<CommandQueue*>(commandQueue);
        if (cmdQueue)
        {
            // GBuffer pass: opaque scene geometry
            cmdQueue->SetGBufferRenderCallback([this]() {
                if (currentScene) currentScene->Render();
            });

            // Shadow pass: scene depth from directional light POV
            cmdQueue->SetShadowRenderCallback([this]() {
                if (!currentScene || !shadowDepthShader) return;
                ZNDirectionalLight* dirLight = currentScene->GetDirectionalLight();
                if (!dirLight) return;
                auto* d3dLight = dynamic_cast<Platform::Direct3D::DirectionalLight*>(dirLight);
                if (d3dLight)
                {
                    ZNMatrix4 lightVP = d3dLight->GetLightViewProjectionMatrix();
                    currentScene->RenderShadow(lightVP, shadowDepthShader);
                }
            });
        }
    }

    // Initialize camera projection when scene is set
    if (currentScene && currentScene->GetCamera() && swapChain)
    {
        ZNCamera* camera = currentScene->GetCamera();
        float aspect = static_cast<float>(swapChain->Width()) / static_cast<float>(swapChain->Height());
        camera->SetPerspective(3.141592f / 4.0f, aspect, 0.1f, 100.0f);
    }
}

ZNScene* ApplicationContext::GetScene() const
{
    return currentScene;
}

void ApplicationContext::OnResize(uint32 width, uint32 height)
{
    if (width == 0 || height == 0)
        return;

    swapChain->Resize(width, height);
    depthStencilBuffer->Init();

    CommandQueue* cmdQueue = dynamic_cast<CommandQueue*>(commandQueue);
    if (cmdQueue)
    {
        GBufferManager* gbufferMgr = cmdQueue->GetGBufferManager();
        if (gbufferMgr)
        {
            gbufferMgr->Resize(width, height);
            cmdQueue->NotifyGBufferResized();
        }
    }

    // Update camera projection from scene
    if (currentScene && currentScene->GetCamera())
    {
        ZNCamera* camera = currentScene->GetCamera();
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        camera->SetPerspective(3.141592f / 4.0f, aspect, 0.1f, 100.0f);
    }
}

void ApplicationContext::OnMouseEvent(struct MouseEvent event)
{
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    if (currentScene && currentScene->GetCamera())
    {
        currentScene->GetCamera()->ProcessMouse(event);
    }
}

void ApplicationContext::OnKeyboardEvent(struct KeyboardEvent event)
{
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    if (currentScene)
    {
        static int callCount = 0;
        if (callCount++ < 5)
        {
            std::cout << "Key Event: type=" << static_cast<int>(event.type)
                      << ", state=" << static_cast<int>(event.state)
                      << ", deltaTime=" << timer->DeltaTime() << std::endl;
        }

        // Forward to scene for custom handling
        currentScene->OnKeyboardEvent(event);

        // Forward to camera for movement
        if (currentScene->GetCamera())
        {
            currentScene->GetCamera()->ProcessKeyboard(event, timer->DeltaTime());
        }
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
    ZNGameObject::FlushDrawCalls();

    if (imguiLayer)
        imguiLayer->BeginFrame();

    // RenderBegin() now runs the full RenderGraph (all passes in order)
    RenderBegin();
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