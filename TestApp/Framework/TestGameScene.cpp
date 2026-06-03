#include "TestGameScene.h"
#include <ZNFramework/Graphics/Platform/GraphicsAPI.h>
#include <iostream>
#include <filesystem>
#include <ZNFramework/Graphics/Platform/Direct3D12/DirectionalLight.h>


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

    Platform::Direct3D::DirectionalLight* d3dDirLight =
        dynamic_cast<Platform::Direct3D::DirectionalLight*>(dirLight);
    if (d3dDirLight)
    {
        d3dDirLight->SetShadowFocusPoint(ZNVector3(0.0f, 0.0f, 0.0f));
        d3dDirLight->SetShadowBounds(50.0f, 0.1f, 100.0f); // 더 넓은 범위
    }

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

                // Red Plastic
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

                ZNMaterial* mat = plasticMat;

                for (const auto& meshData : modelData.meshes)
                {
                    ZNGameObject* obj = new ZNGameObject();
                    ZNMesh* mesh = ZNFramework::Platform::CreateMesh();
                    mesh->Init(meshData.vertices, meshData.indices);
                    mesh->SetMaterial(mat);

                    obj->SetMesh(mesh);
                    obj->GetTransform().rotation = ZNVector3(0.f, 90.f, 0.f);
                    obj->GetTransform().position = ZNVector3(0.0f, 0.0f, 0.0f);
                    obj->GetTransform().scale = ZNVector3(0.01f, 0.01f, 0.01f);

                    AddGameObject(obj);
                    modelObjects.push_back(obj);
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

    // Cube
    {
        cubeMaterial = ZNFramework::Platform::CreateMaterial();
        cubeMaterial->Init();
        cubeMaterial->SetShader(defaultShader);
        MaterialParams cubeParams;
        cubeParams.albedoColor = ZNVector4(0.2f, 0.4f, 0.9f, 1.0f); // 파란색
        cubeParams.metallic = 0.0f;
        cubeParams.roughness = 0.4f;
        cubeParams.ao = 1.0f;
        cubeMaterial->SetParams(cubeParams);

        std::vector<Vertex> cubeVerts;
        std::vector<uint32> cubeIndices;
        ZNVector4 c(1, 1, 1, 1);

        // 각 면을 독립 버텍스로 (노말이 면마다 달라야 하므로)
        auto addFace = [&](ZNVector3 p0, ZNVector3 p1, ZNVector3 p2, ZNVector3 p3, ZNVector3 normal)
        {
            uint32 base = (uint32)cubeVerts.size();
            cubeVerts.push_back(Vertex(p0, c, ZNVector2(0, 0), normal));
            cubeVerts.push_back(Vertex(p1, c, ZNVector2(1, 0), normal));
            cubeVerts.push_back(Vertex(p2, c, ZNVector2(1, 1), normal));
            cubeVerts.push_back(Vertex(p3, c, ZNVector2(0, 1), normal));
            cubeIndices.insert(cubeIndices.end(),
                { base, base + 1, base + 2, base, base + 2, base + 3 });
        };

        // +Y (top)
        addFace({ -1,1,-1 }, { 1,1,-1 }, { 1,1,1 }, { -1,1,1 }, { 0,1,0 });
        // -Y (bottom)
        addFace({ -1,-1,1 }, { 1,-1,1 }, { 1,-1,-1 }, { -1,-1,-1 }, { 0,-1,0 });
        // +Z (front)
        addFace({ -1,-1,1 }, { 1,-1,1 }, { 1,1,1 }, { -1,1,1 }, { 0,0,1 });
        // -Z (back)
        addFace({ 1,-1,-1 }, { -1,-1,-1 }, { -1,1,-1 }, { 1,1,-1 }, { 0,0,-1 });
        // +X (right)
        addFace({ 1,-1,1 }, { 1,-1,-1 }, { 1,1,-1 }, { 1,1,1 }, { 1,0,0 });
        // -X (left)
        addFace({ -1,-1,-1 }, { -1,-1,1 }, { -1,1,1 }, { -1,1,-1 }, { -1,0,0 });

        cube = new ZNGameObject();
        ZNMesh* cubeMesh = ZNFramework::Platform::CreateMesh();
        cubeMesh->Init(cubeVerts, cubeIndices);
        cubeMesh->SetMaterial(cubeMaterial);
        cube->SetMesh(cubeMesh);
        cube->GetTransform().position = ZNVector3(2.5f, 0.5f, 1.5f);
        cube->GetTransform().scale = ZNVector3(0.5f, 0.5f, 0.5f);
        cube->GetTransform().rotation = ZNVector3(0.0f, 30.0f, 0.0f);
        AddGameObject(cube);
    }

    // Sphere (UV sphere)
    {
        sphereMaterial = ZNFramework::Platform::CreateMaterial();
        sphereMaterial->Init();
        sphereMaterial->SetShader(defaultShader);
        MaterialParams sphereParams;
        sphereParams.albedoColor = ZNVector4(0.9f, 0.7f, 0.1f, 1.0f); // 금색
        sphereParams.metallic = 0.8f;
        sphereParams.roughness = 0.2f;
        sphereParams.ao = 1.0f;
        sphereMaterial->SetParams(sphereParams);

        std::vector<Vertex> sphereVerts;
        std::vector<uint32> sphereIndices;

        const int stacks = 16;
        const int slices = 16;
        const float radius = 1.0f;
        const float PI = 3.14159265f;

        for (int i = 0; i <= stacks; ++i)
        {
            float phi = PI * i / stacks; // 0 ~ PI
            for (int j = 0; j <= slices; ++j)
            {
                float theta = 2.0f * PI * j / slices; // 0 ~ 2PI
                float x = radius * sinf(phi) * cosf(theta);
                float y = radius * cosf(phi);
                float z = radius * sinf(phi) * sinf(theta);
                ZNVector3 pos(x, y, z);
                ZNVector3 normal(x, y, z); // 단위 구이므로 pos == normal
                ZNVector2 uv((float)j / slices, (float)i / stacks);
                sphereVerts.push_back(Vertex(pos, ZNVector4(1, 1, 1, 1), uv, normal));
            }
        }

        for (int i = 0; i < stacks; ++i)
        {
            for (int j = 0; j < slices; ++j)
            {
                uint32 a = i * (slices + 1) + j;
                uint32 b = a + 1;
                uint32 c2 = a + (slices + 1);
                uint32 d = c2 + 1;
                sphereIndices.insert(sphereIndices.end(), { a, c2, b, b, c2, d });
            }
        }

        sphere = new ZNGameObject();
        ZNMesh* sphereMesh = ZNFramework::Platform::CreateMesh();
        sphereMesh->Init(sphereVerts, sphereIndices);
        sphereMesh->SetMaterial(sphereMaterial);
        sphere->SetMesh(sphereMesh);
        sphere->GetTransform().position = ZNVector3(-2.0f, 0.5f, 0.0f);
        sphere->GetTransform().scale = ZNVector3(0.5f, 0.5f, 0.5f);
        AddGameObject(sphere);
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

    // Floor Plane - deferred pass로 렌더링하여 그림자를 받음
    {
        floorMaterial = ZNFramework::Platform::CreateMaterial();
        floorMaterial->Init();
        floorMaterial->SetShader(defaultShader);
        MaterialParams floorParams;
        floorParams.albedoColor = ZNVector4(0.6f, 0.6f, 0.6f, 1.0f); // 회색
        floorParams.metallic = 0.0f;
        floorParams.roughness = 0.8f;
        floorParams.ao = 1.0f;
        floorMaterial->SetParams(floorParams);

        std::vector<Vertex> floorVerts;
        std::vector<uint32> floorIndices;
        float s = 10.0f;
        ZNVector4 c(1, 1, 1, 1);
        ZNVector3 n(0, 1, 0);
        floorVerts.push_back(Vertex(ZNVector3(-s, 0, -s), c, ZNVector2(0, 0), n));
        floorVerts.push_back(Vertex(ZNVector3(s, 0, -s), c, ZNVector2(1, 0), n));
        floorVerts.push_back(Vertex(ZNVector3(s, 0, s), c, ZNVector2(1, 1), n));
        floorVerts.push_back(Vertex(ZNVector3(-s, 0, s), c, ZNVector2(0, 1), n));
        floorIndices = { 0, 3, 2, 0, 2, 1 };

        floorPlane = new ZNGameObject();
        ZNMesh* floorMesh = ZNFramework::Platform::CreateMesh();
        floorMesh->Init(floorVerts, floorIndices);
        floorMesh->SetMaterial(floorMaterial);
        floorPlane->SetMesh(floorMesh);
        floorPlane->GetTransform().position = ZNVector3(0.0f, -0.3f, 0.0f);
        floorPlane->SetCastShadow(false);
        AddGameObject(floorPlane);
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
