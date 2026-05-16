#include "TestGameScene.h"
#include <ZNFramework/Graphics/Platform/GraphicsAPI.h>
#include <iostream>
#include <filesystem>

using namespace ZNFramework;

void TestGameScene::Initialize()
{
    // Load default shader
    defaultShader = ZNFramework::Platform::CreateShader();
    std::filesystem::path shaderPath = GetResourcePath() / L"Shaders" / L"deferred_lighting.hlsli";
    defaultShader->Load(shaderPath);

    // Load grid shader
    gridShader = ZNFramework::Platform::CreateShader();
    std::filesystem::path gridShaderPath = GetResourcePath() / L"Shaders" / L"grid.hlsli";
    gridShader->Load(gridShaderPath);
    gridShader->EnableAlphaBlend(); // Enable transparency for grid background

    // Create camera - positioned to look down at floor grid
    ZNCamera* cam = new ZNCamera();
    cam->SetPosition(ZNVector3(0.0f, 3.0f, -8.0f));
    cam->SetRotation(-20, 0, 0);
    cam->SetMoveSpeed(3.0f);
    SetCamera(cam);

    std::cout << "Camera initialized at position: (0, 0, -5)" << std::endl;

    // Setup spot light like a flashlight from camera - GREEN
    ZNSpotLight* spotLight = ZNFramework::Platform::CreateSpotLight();
    ZNVector3 spotLightPos(1.0f, 2.f, 1.0f);
    ZNVector3 spotLightDir(-1.0f, -1.0f, 0.0f);
    spotLightDir = spotLightDir.Normalize();
    spotLight->SetPosition(spotLightPos);
    spotLight->SetDirection(spotLightDir);
    spotLight->SetIntensity(0.5f);
    spotLight->SetColor(ZNVector3(0.0f, 1.0f, 0.0f)); // Green
    spotLight->SetAmbientIntensity(0.1f);
    spotLight->SetCutoffAngle(12.0f, 17.0f);
    spotLight->SetAttenuation(1.0f, 0.045f, 0.0075f);
    //spotLight->SetAttenuation(0.5f, 0.045f, 0.0075f);
    SetLight(spotLight);
    
    // Setup directional light - RED
    ZNDirectionalLight* dirLight = ZNFramework::Platform::CreateDirectionalLight();
    dirLight->SetDirection(ZNVector3(0.5f, -1.0f, 0.3f));
    dirLight->SetIntensity(8.0f);
    dirLight->SetColor(ZNVector3(0.5f, 0.5f, 0.5f));
    dirLight->SetAmbientIntensity(5.0f);
    SetDirectionalLight(dirLight);

    std::cout << "Lights initialized - Spot light (flashlight mode) + Green directional light" << std::endl;

    {
        std::filesystem::path modelPath = GetResourcePath() / L"Models" / L"stanford-bunny.fbx";
        if (std::filesystem::exists(modelPath))
        {
            ZNModelLoader* modelLoader = ZNFramework::Platform::CreateModelLoader();
            ModelData modelData;
            if (modelLoader->Load(modelPath, modelData))
            {
                std::cout << "FBX Model loaded successfully!" << std::endl;

                // ── 머티리얼 3종 ────────────────────────────────────

                // 1. Iron
                ZNMaterial* ironMat = ZNFramework::Platform::CreateMaterial();
                ironMat->Init();
                ironMat->SetShader(defaultShader);
                MaterialParams ironParams;
                ironParams.albedoColor = ZNVector4(0.56f, 0.57f, 0.58f, 1.0f);
                ironParams.metallic = 1.0f;
                ironParams.roughness = 0.8f;
                ironParams.ao = 1.0f;
                ironMat->SetParams(ironParams);
                materials.push_back(ironMat);

                // 2. Gold
                ZNMaterial* goldMat = ZNFramework::Platform::CreateMaterial();
                goldMat->Init();
                goldMat->SetShader(defaultShader);
                MaterialParams goldParams;
                goldParams.albedoColor = ZNVector4(1.0f, 0.78f, 0.34f, 1.0f);
                goldParams.metallic =0.9f;
                goldParams.roughness = 0.1f;
                goldParams.ao = 1.0f;
                goldMat->SetParams(goldParams);
                materials.push_back(goldMat);

                // 3. Red Plastic
                ZNMaterial* plasticMat = ZNFramework::Platform::CreateMaterial();
                plasticMat->Init();
                plasticMat->SetShader(defaultShader);
                MaterialParams plasticParams;
                plasticParams.albedoColor = ZNVector4(0.8f, 0.1f, 0.1f, 1.0f);
                plasticParams.metallic = 0.0f;
                plasticParams.roughness = 0.3f;
                plasticParams.ao = 1.0f;
                plasticMat->SetParams(plasticParams);
                materials.push_back(plasticMat);

                ZNMaterial* mats[3] = { ironMat, goldMat, plasticMat };
                float xPositions[3] = { -3.0f, 0.0f, 3.0f };

                for (int i = 0; i < 3; ++i)
                {
                    for (const auto& meshData : modelData.meshes)
                    {
                        ZNGameObject* obj = new ZNGameObject();
                        ZNMesh* mesh = ZNFramework::Platform::CreateMesh();
                        mesh->Init(meshData.vertices, meshData.indices);
                        mesh->SetMaterial(mats[i]);

                        obj->SetMesh(mesh);
                        obj->GetTransform().rotation = ZNVector3(0.f, 90.f, 0.f);
                        obj->GetTransform().position = ZNVector3(xPositions[i], 0.0f, 0.0f);
                        obj->GetTransform().scale = ZNVector3(0.01f, 0.01f, 0.01f);

                        AddGameObject(obj);
                        modelObjects.push_back(obj);
                    }
                }

                if (!modelObjects.empty())
                    turntableObj = modelObjects.front();

                std::cout << "Created 3 bunnies (Iron / Gold / Plastic)" << std::endl;
                delete modelLoader;
            }
            else
            {
                std::cout << "Failed to load FBX model: " << modelPath << std::endl;
            }
        }
        else
        {
            std::cout << "Model file not found: " << modelPath << std::endl;
        }
    }

    // Debug visualization - Materials
    {
        debugMaterial = ZNFramework::Platform::CreateMaterial();
        debugMaterial->Init();
        debugMaterial->SetShader(defaultShader);
        MaterialParams debugParams;
        debugParams.albedoColor = ZNVector4(1.0f, 1.0f, 0.0f, 1.0f);
        debugParams.metallic = 0.0f;
        debugParams.roughness = 1.0f;
        debugParams.ao = 1.0f;
        debugMaterial->SetParams(debugParams);

        // Grid material for plane
        gridMaterial = ZNFramework::Platform::CreateMaterial();
        gridMaterial->Init();
        gridMaterial->SetShader(gridShader);
        MaterialParams gridParams;
        gridParams.albedoColor = ZNVector4(1.0f, 1.0f, 1.0f, 1.0f);
        gridParams.metallic = 0.0f;
        gridParams.roughness = 1.0f;
        gridParams.ao = 1.0f;
        gridMaterial->SetParams(gridParams);

        // Red material for X axis
        redMaterial = ZNFramework::Platform::CreateMaterial();
        redMaterial->Init();
        redMaterial->SetShader(defaultShader);
        MaterialParams redParams;
        redParams.albedoColor = ZNVector4(1.0f, 1.0f, 0.0f, 1.0f);
        redParams.metallic = 0.0f;
        redParams.roughness = 1.0f;
        redParams.ao = 1.0f;
        redMaterial->SetParams(redParams);

        // Green material for Y axis
        greenMaterial = ZNFramework::Platform::CreateMaterial();
        greenMaterial->Init();
        greenMaterial->SetShader(defaultShader);
        MaterialParams greenParams;
        greenParams.albedoColor = ZNVector4(0.0f, 1.0f, 0.0f, 1.0f);
        greenParams.metallic = 0.0f;
        greenParams.roughness = 1.0f;
        greenParams.ao = 1.0f;
        greenMaterial->SetParams(greenParams);

        // Blue material for Z axis
        blueMaterial = ZNFramework::Platform::CreateMaterial();
        blueMaterial->Init();
        blueMaterial->SetShader(defaultShader);
        MaterialParams blueParams;
        blueParams.albedoColor = ZNVector4(0.0f, 0.0f, 1.0f, 1.0f);
        blueParams.metallic = 0.0f;
        blueParams.roughness = 1.0f;
        blueParams.ao = 1.0f;
        blueMaterial->SetParams(blueParams);
    }

    // Debug visualization - Crosshair
    {
        std::vector<Vertex> crosshairVerts;
        std::vector<uint32> crosshairIndices;

        float length = 0.05f;
        ZNVector4 color(1, 1, 0, 1);
        ZNVector2 uv(0, 0);

        // spotLightDir에 수직인 벡터 계산 (XY 평면에서 90도 회전)
        ZNVector3 perpDir(-spotLightDir.y, spotLightDir.x, 0);
        perpDir = perpDir.Normalize();

        // Main line (spotLightDir 방향)
        crosshairVerts.push_back(Vertex(spotLightDir * -length, color, uv, ZNVector3(0, 0, 1)));
        crosshairVerts.push_back(Vertex(spotLightDir * length, color, uv, ZNVector3(0, 0, 1)));

        // Perpendicular line (수직 방향)
        crosshairVerts.push_back(Vertex(perpDir * -length, color, uv, ZNVector3(0, 0, 1)));
        crosshairVerts.push_back(Vertex(perpDir * length, color, uv, ZNVector3(0, 0, 1)));

        crosshairIndices = { 0, 1, 2, 3 };

        crosshair = new ZNGameObject();
        ZNMesh* crosshairMesh = ZNFramework::Platform::CreateMesh();
        crosshairMesh->Init(crosshairVerts, crosshairIndices);
        crosshairMesh->SetMaterial(debugMaterial);
        crosshair->SetMesh(crosshairMesh);
        crosshair->GetTransform().position = spotLightPos;

        AddGameObject(crosshair);
    }

    // Plane - Horizontal floor grid (XZ plane)
    {
        std::vector<Vertex> planeVerts;
        std::vector<uint32> planeIndices;

        ZNVector4 color(0, 1, 1, 1);
        ZNVector2 uv(0, 0);

        float s = 50.f;
        // Horizontal plane at y=-2 (below camera), facing up (normal pointing +Y)
        planeVerts.push_back(Vertex(ZNVector3(-s, 0, -s), color, uv, ZNVector3(0, 1, 0)));
        planeVerts.push_back(Vertex(ZNVector3(s, 0, -s), color, uv, ZNVector3(0, 1, 0)));
        planeVerts.push_back(Vertex(ZNVector3(s, 0, s), color, uv, ZNVector3(0, 1, 0)));
        planeVerts.push_back(Vertex(ZNVector3(-s, 0, s), color, uv, ZNVector3(0, 1, 0)));

        planeIndices = {0, 3, 2, 0, 2, 1};

        plane = new ZNGameObject();
        ZNMesh* planeMesh = ZNFramework::Platform::CreateMesh();
        planeMesh->Init(planeVerts, planeIndices);
        planeMesh->SetMaterial(gridMaterial);
        plane->SetMesh(planeMesh);
        // Grid uses forward rendering (after deferred lighting pass)
        AddForwardGameObject(plane);
    }
}

void TestGameScene::Update(float deltaTime)
{
    ZNScene::Update(deltaTime);

    if (turntableObj && turntableEnabled)
    {
        turntableObj->GetTransform().rotation.y += 0.5f * deltaTime; // Rotate 20 degrees per second
    }
}

void TestGameScene::OnKeyboardEvent(const ZNFramework::KeyboardEvent& event)
{
    using namespace ZNFramework;

    // Only respond to key down events (not hold/repeat)
    if (event.state != KEY_STATE::DOWN)
        return;

    switch (event.type)
    {
    case KEY_TYPE::KEY_T:
        turntableEnabled = !turntableEnabled;
        std::cout << "Turntable: " << (turntableEnabled ? "ON" : "OFF") << std::endl;
        break;

    case KEY_TYPE::KEY_P:
        planeVisible = !planeVisible;
        if (plane)
        {
            plane->SetVisible(planeVisible);
        }
        std::cout << "Plane: " << (planeVisible ? "VISIBLE" : "HIDDEN") << std::endl;
        break;
    }




}

void TestGameScene::Render()
{
    ZNScene::Render();
}
