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
    dirLight->SetIntensity(0.8f);
    dirLight->SetColor(ZNVector3(0.5f, 0.5f, 0.5f)); // Red
    dirLight->SetAmbientIntensity(0.5f);
    SetDirectionalLight(dirLight);

    std::cout << "Lights initialized - Spot light (flashlight mode) + Green directional light" << std::endl;

    // FBX Model Loading
    {
        std::filesystem::path modelPath = GetResourcePath() / L"Models" / L"stanford-bunny.fbx";

        if (std::filesystem::exists(modelPath))
        {
            ZNModelLoader* modelLoader = ZNFramework::Platform::CreateModelLoader();
            ModelData modelData;

            if (modelLoader->Load(modelPath, modelData))
            {
                std::cout << "FBX Model loaded successfully!" << std::endl;
                std::cout << "  Meshes: " << modelData.meshes.size() << std::endl;
                std::cout << "  Materials: " << modelData.materials.size() << std::endl;

                // Create materials from loaded data
                for (const auto& matData : modelData.materials)
                {
                    ZNMaterial* material = ZNFramework::Platform::CreateMaterial();
                    material->Init();
                    material->SetShader(defaultShader);

                    // Use material params from FBX
                    MaterialParams params = matData.params;
                    params.metallic = 0.0f;
                    params.roughness = 0.8f;
                    material->SetParams(params);

                    // Load textures
                    for (size_t i = 0; i < static_cast<size_t>(TextureType::Count); ++i)
                    {
                        if (!matData.texturePaths[i].empty())
                        {
                            ZNTexture* texture = ZNFramework::Platform::CreateTexture();
                            texture->Init(matData.texturePaths[i]);
                            material->SetTexture(static_cast<TextureType>(i), texture);
                            textures.push_back(texture);
                        }
                    }

                    materials.push_back(material);
                }

                // Create game objects from meshes
                for (const auto& meshData : modelData.meshes)
                {
                    ZNGameObject* obj = new ZNGameObject();
                    ZNMesh* mesh = ZNFramework::Platform::CreateMesh();
                    mesh->Init(meshData.vertices, meshData.indices);

                    if (meshData.materialIndex < materials.size())
                    {
                        mesh->SetMaterial(materials[meshData.materialIndex]);
                    }

                    obj->SetMesh(mesh);
                    obj->GetTransform().position = ZNVector3(0.0f, 0.0f, 100.0f);
                    obj->GetTransform().scale = ZNVector3(0.01f, 0.01f, 0.01f);

                    AddGameObject(obj);
                    modelObjects.push_back(obj);
                }

                std::cout << "Created " << modelObjects.size() << " game objects from FBX model" << std::endl;
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
}

void TestGameScene::Render()
{
    ZNScene::Render();
}
