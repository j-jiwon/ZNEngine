#ifdef _WIN32
#include <Windows.h>
#include <iostream>
#include "ApplicationContext.h"
#include "ZNFramework.h"
#include "ZNFramework/Graphics/Platform/GraphicsAPI.h"
#include "ZNFramework/Graphics/ZNModelLoader.h"

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
    defaultMesh = ZNFramework::Platform::CreateMesh();
    defaultTexture = ZNFramework::Platform::CreateTexture();
    defaultMaterial = ZNFramework::Platform::CreateMaterial();

    constantBuffer = ZNFramework::Platform::CreateConstantBuffer();
    GraphicsContext::GetInstance().SetConstantBuffer(constantBuffer);
    
    tableDescriptorHeap = ZNFramework::Platform::CreateTableDescriptorHeap();
    GraphicsContext::GetInstance().SetTableDescriptorHeap(tableDescriptorHeap);

    depthStencilBuffer = ZNFramework::Platform::CreateDepthStencilBuffer();
    GraphicsContext::GetInstance().SetDepthStencilBuffer(depthStencilBuffer);

    // Create camera
    camera = new ZNCamera();
    camera->SetPosition(ZNVector3(0.0f, 0.0f, -5.0f));
    camera->SetRotation(0.0f, 0.0f);
    camera->SetMoveSpeed(3.0f);
    GraphicsContext::GetInstance().SetCamera(camera);

    std::cout << "Camera initialized at position: (0, 0, -5)" << std::endl;

    // Setup spot light like a flashlight from camera
    spotLight = ZNFramework::Platform::CreateSpotLight();
    spotLight->SetPosition(camera->GetPosition()); // At camera position
    spotLight->SetDirection(ZNVector3(0.0f, 0.0f, 1.0f)); // Camera forward direction (+Z)
    spotLight->SetIntensity(1.0f); // Bright but balanced with ambient
    spotLight->SetColor(ZNVector3(0.8f, 0.1f, 0.1f)); // Warm white/yellow
    spotLight->SetAmbientIntensity(0.2f); // Moderate ambient so models are visible
    spotLight->SetCutoffAngle(5.0f, 15.0f); // Wider cone
    spotLight->SetAttenuation(1.0f, 0.045f, 0.0075f); // Less attenuation for longer range
    GraphicsContext::GetInstance().SetLight(spotLight);

    // Setup green directional light
    directionalLight = ZNFramework::Platform::CreateDirectionalLight();
    directionalLight->SetDirection(ZNVector3(0.5f, -1.0f, 0.3f)); // Coming from upper right
    directionalLight->SetIntensity(0.8f); // Medium intensity
    directionalLight->SetColor(ZNVector3(0.2f, 1.0f, 0.2f)); // Green color
    directionalLight->SetAmbientIntensity(0.0f); // No ambient, just directional
    GraphicsContext::GetInstance().SetDirectionalLight(directionalLight);

    std::cout << "Lights initialized - Spot light (flashlight mode) + Green directional light" << std::endl;

    // initialize
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

        // Setup Material
        defaultMaterial->Init();
        defaultMaterial->SetShader(defaultShader);
        defaultMaterial->SetTexture(TextureType::Albedo, defaultTexture);

        MaterialParams params;
        params.albedoColor = ZNVector4(1.f, 0.f, 0.f, 1.f);  // White with full opacity
        params.metallic = 1.0f;
        params.roughness = 0.5f;
        params.ao = 1.0f;
        defaultMaterial->SetParams(params);
    }

    // FBX Model Test - Load a model from Resources/Models/
    // Put your FBX file in c:\Work\ZNEngine\Resources\Models\
    // Example: c:\Work\ZNEngine\Resources\Models\test.fbx
    {
        std::filesystem::path modelPath = GetResourcePath() / L"Models" / L"stanford-bunny.fbx";

        // Check if the model file exists
        if (std::filesystem::exists(modelPath))
        {
            ZNModelLoader* modelLoader = ZNFramework::Platform::CreateModelLoader();
            ModelData modelData;

            if (modelLoader->Load(modelPath, modelData))
            {
                std::cout << "FBX Model loaded successfully!" << std::endl;
                std::cout << "  Meshes: " << modelData.meshes.size() << std::endl;
                std::cout << "  Materials: " << modelData.materials.size() << std::endl;

                // Debug: Print mesh info
                for (size_t i = 0; i < modelData.meshes.size(); ++i)
                {
                    std::cout << "  Mesh " << i << ": "
                              << modelData.meshes[i].vertices.size() << " vertices, "
                              << modelData.meshes[i].indices.size() << " indices" << std::endl;
                }

                // Create materials from loaded data
                for (const auto& matData : modelData.materials)
                {
                    ZNMaterial* material = ZNFramework::Platform::CreateMaterial();
                    material->Init();
                    material->SetShader(defaultShader); // Use default shader

                    // Use material params from FBX
                    MaterialParams params = matData.params;
                    // Keep original albedo color from FBX file
                    params.metallic = 0.0f; // Non-metallic for better diffuse lighting
                    params.roughness = 0.8f;
                    material->SetParams(params);

                    // Load textures for this material
                    for (size_t i = 0; i < static_cast<size_t>(TextureType::Count); ++i)
                    {
                        if (!matData.texturePaths[i].empty())
                        {
                            ZNTexture* texture = ZNFramework::Platform::CreateTexture();
                            texture->Init(matData.texturePaths[i]);
                            material->SetTexture(static_cast<TextureType>(i), texture);
                            loadedTextures.push_back(texture);
                        }
                    }

                    loadedMaterials.push_back(material);
                }

                // Create meshes from loaded data
                for (const auto& meshData : modelData.meshes)
                {
                    ZNMesh* mesh = ZNFramework::Platform::CreateMesh();
                    mesh->Init(meshData.vertices, meshData.indices);

                    // Assign material to mesh, fallback to default material
                    if (meshData.materialIndex < loadedMaterials.size() && loadedMaterials[meshData.materialIndex])
                    {
                        mesh->SetMaterial(loadedMaterials[meshData.materialIndex]);
                    }
                    else
                    {
                        // Use default material with white color if model has no material
                        mesh->SetMaterial(defaultMaterial);
                    }

                    loadedMeshes.push_back(mesh);
                }
            }
            else
            {
                std::cout << "Failed to load FBX model: " << modelPath << std::endl;
            }

            delete modelLoader;
        }
        else
        {
            std::cout << "FBX Model not found: " << modelPath << std::endl;
            std::cout << "To test the camera with a 3D model:" << std::endl;
            std::cout << "  1. Place your FBX file at: " << modelPath << std::endl;
            std::cout << "  2. Or change the filename in ApplicationContext.cpp (line ~160)" << std::endl;
        }
    }

    // Create debug visualization meshes
    {
        // Create debug material (solid color, no lighting needed)
        debugMaterial = ZNFramework::Platform::CreateMaterial();
        debugMaterial->Init();
        debugMaterial->SetShader(defaultShader);
        MaterialParams debugParams;
        debugParams.albedoColor = ZNVector4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
        debugParams.metallic = 0.0f;
        debugParams.roughness = 1.0f;
        debugParams.ao = 1.0f;
        debugMaterial->SetParams(debugParams);

        // Create light debug cube (small cube at light position)
        {
            std::vector<Vertex> cubeVerts;
            std::vector<uint32> cubeIndices;

            float s = 0.1f; // Cube half-size
            ZNVector4 color(1, 0, 0, 1); // Yellow
            ZNVector2 uv(0, 0);

            // Front face (z = s)
            cubeVerts.push_back(Vertex(ZNVector3(-s, -s, s), color, uv, ZNVector3(0, 0, 1)));
            cubeVerts.push_back(Vertex(ZNVector3(s, -s, s), color, uv, ZNVector3(0, 0, 1)));
            cubeVerts.push_back(Vertex(ZNVector3(s, s, s), color, uv, ZNVector3(0, 0, 1)));
            cubeVerts.push_back(Vertex(ZNVector3(-s, s, s), color, uv, ZNVector3(0, 0, 1)));

            // Back face (z = -s)
            cubeVerts.push_back(Vertex(ZNVector3(s, -s, -s), color, uv, ZNVector3(0, 0, -1)));
            cubeVerts.push_back(Vertex(ZNVector3(-s, -s, -s), color, uv, ZNVector3(0, 0, -1)));
            cubeVerts.push_back(Vertex(ZNVector3(-s, s, -s), color, uv, ZNVector3(0, 0, -1)));
            cubeVerts.push_back(Vertex(ZNVector3(s, s, -s), color, uv, ZNVector3(0, 0, -1)));

            // Top face (y = s)
            cubeVerts.push_back(Vertex(ZNVector3(-s, s, s), color, uv, ZNVector3(0, 1, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(s, s, s), color, uv, ZNVector3(0, 1, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(s, s, -s), color, uv, ZNVector3(0, 1, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(-s, s, -s), color, uv, ZNVector3(0, 1, 0)));

            // Bottom face (y = -s)
            cubeVerts.push_back(Vertex(ZNVector3(-s, -s, -s), color, uv, ZNVector3(0, -1, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(s, -s, -s), color, uv, ZNVector3(0, -1, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(s, -s, s), color, uv, ZNVector3(0, -1, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(-s, -s, s), color, uv, ZNVector3(0, -1, 0)));

            // Right face (x = s)
            cubeVerts.push_back(Vertex(ZNVector3(s, -s, s), color, uv, ZNVector3(1, 0, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(s, -s, -s), color, uv, ZNVector3(1, 0, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(s, s, -s), color, uv, ZNVector3(1, 0, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(s, s, s), color, uv, ZNVector3(1, 0, 0)));

            // Left face (x = -s)
            cubeVerts.push_back(Vertex(ZNVector3(-s, -s, -s), color, uv, ZNVector3(-1, 0, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(-s, -s, s), color, uv, ZNVector3(-1, 0, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(-s, s, s), color, uv, ZNVector3(-1, 0, 0)));
            cubeVerts.push_back(Vertex(ZNVector3(-s, s, -s), color, uv, ZNVector3(-1, 0, 0)));

            // Indices for all 6 faces (2 triangles per face)
            for (uint32 i = 0; i < 6; ++i) {
                uint32 base = i * 4;
                cubeIndices.push_back(base + 0);
                cubeIndices.push_back(base + 1);
                cubeIndices.push_back(base + 2);
                cubeIndices.push_back(base + 0);
                cubeIndices.push_back(base + 2);
                cubeIndices.push_back(base + 3);
            }

            lightDebugMesh = ZNFramework::Platform::CreateMesh();
            lightDebugMesh->Init(cubeVerts, cubeIndices);
            lightDebugMesh->SetMaterial(debugMaterial);
        }

        std::cout << "Debug visualization meshes created (including XYZ axes)" << std::endl;
    }

    commandQueue->WaitSync();
}

void ApplicationContext::OnResize(uint32 width, uint32 height)
{
    swapChain->Resize(width, height);
    depthStencilBuffer->Init();

    // Update camera projection
    if (camera && width > 0 && height > 0)
    {
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        camera->SetPerspective(3.141592f / 4.0f, aspect, 0.1f, 1000.0f); // 45 degrees FOV
    }
}

void ApplicationContext::OnMouseEvent(MouseEvent event)
{
    if (camera)
    {
        camera->ProcessMouse(event, 0.002f);
    }
}

void ApplicationContext::OnKeyboardEvent(KeyboardEvent event)
{
    if (camera && timer)
    {
        static int callCount = 0;
        if (callCount++ < 5)  // Print first 5 events
        {
            std::cout << "Key Event: type=" << static_cast<int>(event.type)
                      << ", state=" << static_cast<int>(event.state)
                      << ", deltaTime=" << timer->DeltaTime() << std::endl;
        }
        camera->ProcessKeyboard(event, timer->DeltaTime());
    }
}

void ApplicationContext::Update()
{
    if (camera)
    {
        camera->UpdateViewMatrix();

        // Update spot light to follow camera (flashlight effect)
        ZNLight* light = GraphicsContext::GetInstance().GetLight();
        if (light && light->GetType() == LightType::Spot)
        {
            ZNSpotLight* spotLight = static_cast<ZNSpotLight*>(light);
            spotLight->SetPosition(camera->GetPosition());

            // Calculate camera forward direction from rotation
            float pitch = camera->GetPitch();
            float yaw = camera->GetYaw();

            ZNVector3 forward;
            forward.x = cos(pitch) * sin(yaw);
            forward.y = sin(pitch);
            forward.z = cos(pitch) * cos(yaw);

            spotLight->SetDirection(forward);
        }
    }
}

void ApplicationContext::Render()
{
    RenderBegin();

    // Render test quad (uncomment to debug)
    /*
    defaultMesh->SetMaterial(defaultMaterial);
    {
        Transform t;
        t.position = ZNVector3(-1.0f, 0.0f, 2.0f);  // To the left
        t.scale = ZNVector3(0.5f, 0.5f, 0.5f);
        defaultMesh->SetTransform(t);
        defaultMesh->Render();
    }
    */

    // Render loaded FBX model
    if (!loadedMeshes.empty())
    {
        static bool printedOnce = false;
        if (!printedOnce)
        {
            std::cout << "Rendering " << loadedMeshes.size() << " meshes" << std::endl;
            printedOnce = true;
        }

        for (auto* mesh : loadedMeshes)
        {
            Transform modelTransform;
            modelTransform.position = ZNVector3(0.0f, 0.0f, 0.0f);  // At origin
            modelTransform.scale = ZNVector3(0.01f, 0.01f, 0.01f);  // Small scale
            mesh->SetTransform(modelTransform);
            mesh->Render();
        }
    }
    else
    {
        static bool printedOnce = false;
        if (!printedOnce)
        {
            std::cout << "No meshes loaded to render" << std::endl;
            printedOnce = true;
        }
    }

    // Render debug visualizations
    if (camera && lightDebugMesh && crosshairMesh)
    {
        // Render crosshair at screen center (in front of camera)
        {
            Transform crosshairTransform;
            ZNVector3 forward(0, 0, 1); // Camera forward is +Z
            ZNVector3 right(1, 0, 0);
            ZNVector3 up(0, 1, 0);

            // Position crosshair 1 unit in front of camera
            crosshairTransform.position = camera->GetPosition() + forward * 1.0f;
            crosshairTransform.scale = ZNVector3(1.0f, 1.0f, 1.0f);

            crosshairMesh->SetTransform(crosshairTransform);
            crosshairMesh->Render();
        }

        // Render light position indicator (slightly ahead of camera)
        {
            ZNLight* light = GraphicsContext::GetInstance().GetLight();
            if (light && light->GetType() == LightType::Spot)
            {
                ZNSpotLight* spotLight = static_cast<ZNSpotLight*>(light);
                Transform lightTransform;
                // Show light box 2 units ahead of camera
                lightTransform.position = ZNVector3(5, 0, 15); //spotLight->GetPosition() + camera->GetForward() * 2.0f;
                lightTransform.scale = ZNVector3(0.5f, 0.5f, 0.5f); // Smaller box

                lightDebugMesh->SetTransform(lightTransform);
                lightDebugMesh->Render();
            }
        }

        // Render XYZ axes at origin
        if (axisXMesh && axisYMesh && axisZMesh)
        {
            Transform axisTransform;
            axisTransform.position = ZNVector3(0.0f, 0.0f, 0.0f); // At world origin
            axisTransform.scale = ZNVector3(1.0f, 1.0f, 1.0f);
            axisTransform.rotation = ZNVector3(0.0f, 0.0f, 0.0f);

            // Render X axis (Red)
            axisXMesh->SetTransform(axisTransform);
            axisXMesh->Render();

            // Render Y axis (Green)
            axisYMesh->SetTransform(axisTransform);
            axisYMesh->Render();

            // Render Z axis (Blue)
            axisZMesh->SetTransform(axisTransform);
            axisZMesh->Render();
        }
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