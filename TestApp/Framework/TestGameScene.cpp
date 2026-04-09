#include "TestGameScene.h"
#include <ZNFramework/Graphics/Platform/GraphicsAPI.h>
#include <iostream>
#include <filesystem>

using namespace ZNFramework;

void TestGameScene::Initialize()
{
    // Load default shader
    defaultShader = ZNFramework::Platform::CreateShader();
    std::filesystem::path shaderPath = GetResourcePath() / L"Shaders" / L"default.hlsli";
    defaultShader->Load(shaderPath);

    // Create camera
    ZNCamera* cam = new ZNCamera();
    cam->SetPosition(ZNVector3(0.0f, 0.0f, -5.0f));
    cam->SetRotation(0.0f, 0.0f);
    cam->SetMoveSpeed(3.0f);
    SetCamera(cam);

    std::cout << "Camera initialized at position: (0, 0, -5)" << std::endl;

    // Setup spot light like a flashlight from camera - GREEN
    ZNSpotLight* spotLight = ZNFramework::Platform::CreateSpotLight();
    spotLight->SetPosition(cam->GetPosition()); // Start at camera position
    spotLight->SetDirection(ZNVector3(0.0f, 0.0f, 1.0f));
    spotLight->SetIntensity(0.5f);
    spotLight->SetColor(ZNVector3(0.0f, 1.0f, 0.0f)); // Green
    spotLight->SetAmbientIntensity(0.1f);
    spotLight->SetCutoffAngle(0.0f, 5.0f);
    // spotLight->SetAttenuation(1.0f, 0.045f, 0.0075f);
    spotLight->SetAttenuation(0.5f, 0.045f, 0.0075f);
    SetLight(spotLight);

    // Setup directional light - RED
    ZNDirectionalLight* dirLight = ZNFramework::Platform::CreateDirectionalLight();
    dirLight->SetDirection(ZNVector3(0.5f, -1.0f, 0.3f));
    dirLight->SetIntensity(0.8f);
    dirLight->SetColor(ZNVector3(1.0f, 0.0f, 0.0f)); // Red
    dirLight->SetAmbientIntensity(0.0f);
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
        debugParams.albedoColor = ZNVector4(1.0f, 1.0f, 1.0f, 1.0f);
        debugParams.metallic = 0.0f;
        debugParams.roughness = 1.0f;
        debugParams.ao = 1.0f;
        debugMaterial->SetParams(debugParams);

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

        float size = 0.02f;
        float length = 0.05f;
        ZNVector4 color(1, 1, 1, 1);
        ZNVector2 uv(0, 0);

        // Horizontal line (left-right)
        crosshairVerts.push_back(Vertex(ZNVector3(-length, 0, 0), color, uv, ZNVector3(0, 0, 1)));
        crosshairVerts.push_back(Vertex(ZNVector3(length, 0, 0), color, uv, ZNVector3(0, 0, 1)));

        // Vertical line (up-down)
        crosshairVerts.push_back(Vertex(ZNVector3(0, -length, 0), color, uv, ZNVector3(0, 0, 1)));
        crosshairVerts.push_back(Vertex(ZNVector3(0, length, 0), color, uv, ZNVector3(0, 0, 1)));

        crosshairIndices = { 0, 1, 2, 3 };

        crosshair = new ZNGameObject();
        ZNMesh* crosshairMesh = ZNFramework::Platform::CreateMesh();
        crosshairMesh->Init(crosshairVerts, crosshairIndices);
        crosshairMesh->SetMaterial(debugMaterial);
        crosshair->SetMesh(crosshairMesh);
        AddGameObject(crosshair);
    }

    // Debug visualization - Light indicator (small cube)
    {
        std::vector<Vertex> cubeVerts;
        std::vector<uint32> cubeIndices;

        float s = 0.1f;
        ZNVector4 color(1, 1, 0, 1);
        ZNVector2 uv(0, 0);

        // Front face
        cubeVerts.push_back(Vertex(ZNVector3(-s, -s, -s), color, uv, ZNVector3(0, 0, -1)));
        cubeVerts.push_back(Vertex(ZNVector3(s, -s, -s), color, uv, ZNVector3(0, 0, -1)));
        cubeVerts.push_back(Vertex(ZNVector3(s, s, -s), color, uv, ZNVector3(0, 0, -1)));
        cubeVerts.push_back(Vertex(ZNVector3(-s, s, -s), color, uv, ZNVector3(0, 0, -1)));

        // Back face (reversed vertex order for correct CW winding)
        cubeVerts.push_back(Vertex(ZNVector3(s, -s, s), color, uv, ZNVector3(0, 0, 1)));
        cubeVerts.push_back(Vertex(ZNVector3(-s, -s, s), color, uv, ZNVector3(0, 0, 1)));
        cubeVerts.push_back(Vertex(ZNVector3(-s, s, s), color, uv, ZNVector3(0, 0, 1)));
        cubeVerts.push_back(Vertex(ZNVector3(s, s, s), color, uv, ZNVector3(0, 0, 1)));

        // Top face
        cubeVerts.push_back(Vertex(ZNVector3(-s, s, -s), color, uv, ZNVector3(0, 1, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(s, s, -s), color, uv, ZNVector3(0, 1, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(s, s, s), color, uv, ZNVector3(0, 1, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(-s, s, s), color, uv, ZNVector3(0, 1, 0)));

        // Bottom face
        cubeVerts.push_back(Vertex(ZNVector3(-s, -s, -s), color, uv, ZNVector3(0, -1, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(s, -s, -s), color, uv, ZNVector3(0, -1, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(s, -s, s), color, uv, ZNVector3(0, -1, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(-s, -s, s), color, uv, ZNVector3(0, -1, 0)));

        // Right face (reversed vertex order for correct CW winding)
        cubeVerts.push_back(Vertex(ZNVector3(s, -s, -s), color, uv, ZNVector3(1, 0, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(s, -s, s), color, uv, ZNVector3(1, 0, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(s, s, s), color, uv, ZNVector3(1, 0, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(s, s, -s), color, uv, ZNVector3(1, 0, 0)));

        // Left face (reversed vertex order for correct CW winding)
        cubeVerts.push_back(Vertex(ZNVector3(-s, -s, s), color, uv, ZNVector3(-1, 0, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(-s, -s, -s), color, uv, ZNVector3(-1, 0, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(-s, s, -s), color, uv, ZNVector3(-1, 0, 0)));
        cubeVerts.push_back(Vertex(ZNVector3(-s, s, s), color, uv, ZNVector3(-1, 0, 0)));

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

        lightIndicator = new ZNGameObject();
        ZNMesh* lightMesh = ZNFramework::Platform::CreateMesh();
        lightMesh->Init(cubeVerts, cubeIndices);
        lightMesh->SetMaterial(debugMaterial);
        lightIndicator->SetMesh(lightMesh);
        AddGameObject(lightIndicator);
    }

}

void TestGameScene::Update(float deltaTime)
{
    // Call base class update
    ZNScene::Update(deltaTime);

    // Update crosshair position (1 unit in front of camera)
    if (crosshair && camera)
    {
        ZNVector3 camPos = camera->GetPosition();
        ZNVector3 camForward = camera->GetForward();
        crosshair->GetTransform().position = camPos + camForward * 1.0f;
    }

    // Update light indicator position (2 units ahead of camera for spot light)
    if (lightIndicator && camera)
    {
        ZNVector3 camPos = camera->GetPosition();
        ZNVector3 camForward = camera->GetForward();
        lightIndicator->GetTransform().position = camPos + camForward * 2.0f;
    }
}

void TestGameScene::Render()
{
    // Call base class render
    ZNScene::Render();
}
