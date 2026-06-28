#include "TestGameScene.h"
#include "SceneDebugUI.h"
#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <imgui.h>
#include "ZNFramework/Graphics/Platform/Direct3D12/RenderTexture.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/Material.h"

using namespace ZNFramework;

void TestGameScene::Initialize()
{
    // Load shaders
    defaultShader = Platform::CreateShader();
    defaultShader->Load(GetResourcePath() / L"Shaders" / L"deferred_lighting.hlsli");

    gridShader = Platform::CreateShader();
    gridShader->Load(GetResourcePath() / L"Shaders" / L"grid.hlsli");
    gridShader->EnableAlphaBlend();

    // Camera
    ZNCamera* cam = new ZNCamera();
    cam->SetPosition(ZNVector3(0.0f, 3.0f, -8.0f));
    cam->SetRotation(-20, 0, 0);
    cam->SetMoveSpeed(3.0f);
    SetCamera(cam);

    // Spot light 1 (Green)
    ZNSpotLight* spotLight1 = Platform::CreateSpotLight();
    spotLight1->SetPosition(ZNVector3(1.0f, 2.0f, 1.0f));
    spotLight1->SetDirection(ZNVector3(-1.0f, -1.0f, 0.0f).Normalize());
    spotLight1->SetIntensity(0.5f);
    spotLight1->SetColor(ZNVector3(0.0f, 1.0f, 0.0f));
    spotLight1->SetAmbientIntensity(0.1f);
    spotLight1->SetCutoffAngle(12.0f, 17.0f);
    spotLight1->SetAttenuation(0.5f, 0.045f, 0.0075f);
    AddSpotLight(spotLight1);

    // Spot light 2 (Red)
    ZNSpotLight* spotLight2 = Platform::CreateSpotLight();
    spotLight2->SetPosition(ZNVector3(-3.0f, 3.0f, -2.0f));
    spotLight2->SetDirection(ZNVector3(1.0f, -2.f, 1.0f).Normalize());
    spotLight2->SetIntensity(0.8f);
    spotLight2->SetColor(ZNVector3(1.0f, 0.0f, 0.0f));
    spotLight2->SetAmbientIntensity(1.0f);
    spotLight2->SetCutoffAngle(8.0f, 25.0f);
    spotLight2->SetAttenuation(0.5f, 0.045f, 0.0075f);
    AddSpotLight(spotLight2);

    // Directional light with shadow
    ZNDirectionalLight* dirLight = Platform::CreateDirectionalLight();
    dirLight->SetDirection(ZNVector3(0.5f, -1.0f, 0.3f));
    dirLight->SetIntensity(6.0f);
    dirLight->SetColor(ZNVector3(0.5f, 0.5f, 0.5f));
    dirLight->SetAmbientIntensity(1.0f);
    dirLight->SetShadowFocusPoint(ZNVector3(0.0f, 0.0f, 0.0f));
    dirLight->SetShadowBounds(50.0f, 0.1f, 100.0f);
    SetDirectionalLight(dirLight);

    // Load bunny model
    std::filesystem::path modelPath = GetResourcePath() / L"Models" / L"stanford-bunny.fbx";
    if (std::filesystem::exists(modelPath))
    {
        ZNModelLoader* loader = Platform::CreateModelLoader();
        ModelData modelData;
        if (loader->Load(modelPath, modelData))
        {
            ZNMaterial* bunnyMat = ZNMaterialFactory::CreatePBR(defaultShader,
                ZNVector4(0.8f, 0.1f, 0.1f, 1.0f), 0.0f, 0.3f);
            models.materials.push_back(bunnyMat);

            for (const auto& meshData : modelData.meshes)
            {
                ZNGameObject* obj = new ZNGameObject();
                ZNMesh* mesh = Platform::CreateMesh();
                mesh->Init(meshData.vertices, meshData.indices);
                mesh->SetMaterial(bunnyMat);

                obj->SetMesh(mesh);
                obj->SetMaterial(bunnyMat);
                obj->SetName("Bunny");
                obj->GetTransform().rotation = ZNVector3(0.f, 90.f, 0.f);
                obj->GetTransform().scale = ZNVector3(0.0001f, 0.0001f, 0.0001f);

                AddGameObject(obj);
                models.objects.push_back(obj);
            }

            if (!models.objects.empty())
                turntableObj = models.objects.front();
        }
        delete loader;
    }

    // Scene: Floor
    scene.floorMaterial = ZNMaterialFactory::CreatePBR(defaultShader,
        ZNVector4(0.6f, 0.6f, 0.6f, 1.0f), 0.0f, 0.8f);
    scene.floor = new ZNGameObject();
    scene.floor->SetMesh(ZNMeshFactory::CreatePlane(10.0f));
    scene.floor->GetMesh()->SetMaterial(scene.floorMaterial);
    scene.floor->SetMaterial(scene.floorMaterial);
    scene.floor->SetName("Floor");
    scene.floor->GetTransform().position = ZNVector3(0.0f, -0.3f, 0.0f);
    scene.floor->SetCastShadow(false);
    AddGameObject(scene.floor);

    // Scene: Cube
    scene.cubeMaterial = ZNMaterialFactory::CreatePBR(defaultShader,
        ZNVector4(0.2f, 0.4f, 0.9f, 1.0f), 0.0f, 0.4f);
    scene.cube = new ZNGameObject();
    scene.cube->SetMesh(ZNMeshFactory::CreateCube(1.0f));
    scene.cube->GetMesh()->SetMaterial(scene.cubeMaterial);
    scene.cube->SetMaterial(scene.cubeMaterial);
    scene.cube->SetName("Cube");
    scene.cube->GetTransform().position = ZNVector3(2.5f, 0.5f, 1.5f);
    scene.cube->GetTransform().scale = ZNVector3(0.5f, 0.5f, 0.5f);
    scene.cube->GetTransform().rotation = ZNVector3(0.0f, 30.0f, 0.0f);
    AddGameObject(scene.cube);

    // Scene: Sphere
    scene.sphereMaterial = ZNMaterialFactory::CreatePBR(defaultShader,
        ZNVector4(0.9f, 0.7f, 0.1f, 1.0f), 0.8f, 0.2f);
    scene.sphere = new ZNGameObject();
    scene.sphere->SetMesh(ZNMeshFactory::CreateSphere(1.0f, 16, 16));
    scene.sphere->GetMesh()->SetMaterial(scene.sphereMaterial);
    scene.sphere->SetMaterial(scene.sphereMaterial);
    scene.sphere->SetName("Sphere");
    scene.sphere->GetTransform().position = ZNVector3(-2.0f, 0.5f, 0.0f);
    scene.sphere->GetTransform().scale = ZNVector3(0.5f, 0.5f, 0.5f);
    AddGameObject(scene.sphere);

    // Debug: Grid plane
    debug.gridMaterial = ZNMaterialFactory::CreatePBR(gridShader,
        ZNVector4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 1.0f);
    debug.gridPlane = new ZNGameObject();
    debug.gridPlane->SetMesh(ZNMeshFactory::CreatePlane(50.0f));
    debug.gridPlane->GetMesh()->SetMaterial(debug.gridMaterial);
    debug.gridPlane->SetName("GridPlane");
    debug.gridPlane->SetTag("Debug");
    debug.gridPlane->SetVisible(false);
    AddForwardGameObject(debug.gridPlane);

    // --- CCTV multi-camera demo ---
    cctv.rt = new RenderTexture();
    cctv.rt->Init(512, 288);

    cctv.camera = new ZNCamera();
    cctv.camera->SetPosition(ZNVector3(6.0f, 4.0f, 0.0f));
    cctv.camera->SetRotation(-25.0f, -100.0f);
    cctv.camera->SetPerspective(3.141592f / 4.0f, 512.0f / 288.0f, 0.1f, 100.0f);

    cctv.fwdShader = Platform::CreateShader();
    cctv.fwdShader->Load(GetResourcePath() / L"Shaders" / L"forward_pbr.hlsli");

    cctv.tvUnlitShader = Platform::CreateShader();
    cctv.tvUnlitShader->Load(GetResourcePath() / L"Shaders" / L"screen_unlit.hlsli");

    cctv.tvMat = ZNMaterialFactory::CreatePBR(cctv.tvUnlitShader, ZNVector4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 1.0f);
    static_cast<Material*>(cctv.tvMat)->SetAlbedoSRVHandle(cctv.rt->GetSRVCpuHandle());

    cctv.tvObj = new ZNGameObject();
    cctv.tvObj->SetMesh(ZNMeshFactory::CreatePlane(0.5f));
    cctv.tvObj->GetMesh()->SetMaterial(cctv.tvMat);
    cctv.tvObj->SetMaterial(cctv.tvMat);
    cctv.tvObj->SetName("TV");
    cctv.tvObj->SetTag("TV");
    cctv.tvObj->GetTransform().position = ZNVector3(-1.0f, 2.0f, 4.0f);
    cctv.tvObj->GetTransform().scale    = ZNVector3(3.2f, 0.1f, 1.8f);
    cctv.tvObj->GetTransform().rotation = ZNVector3(-90.0f, 0.0f, 0.0f);
    cctv.tvObj->SetCastShadow(false);
    AddForwardGameObject(cctv.tvObj);

    AddOffscreenCamera(cctv.camera, cctv.rt, "CCTV", cctv.fwdShader);

    // Register CCTV camera for SceneDebugUI's common camera indicator
    RegisterDebugCamera(cctv.camera, "CCTV Camera");

    std::cout << "Scene initialized. Press F1 to toggle debug visuals." << std::endl;
}

void TestGameScene::Update(float deltaTime)
{
    ZNScene::Update(deltaTime);

    if (turntableObj && turntableEnabled)
        turntableObj->GetTransform().rotation.y += 45.0f * deltaTime;
}

void TestGameScene::OnKeyboardEvent(const KeyboardEvent& event)
{
    if (event.state != KEY_STATE::DOWN)
        return;

    switch (event.type)
    {
    case KEY_TYPE::KEY_F1:
        ToggleDebugVisuals();
        break;

    case KEY_TYPE::KEY_T:
        turntableEnabled = !turntableEnabled;
        std::cout << "Turntable: " << (turntableEnabled ? "ON" : "OFF");
        if (turntableObj)
            std::cout << " (" << turntableObj->GetName() << ")";
        std::cout << std::endl;
        break;
    }
}

void TestGameScene::ToggleDebugVisuals()
{
    bool anyVisible = debug.showGrid
        || SceneDebugUI::Get().showSpotIndicators
        || SceneDebugUI::Get().showCamIndicators;

    debug.showGrid = !anyVisible;
    SceneDebugUI::Get().showSpotIndicators = !anyVisible;
    SceneDebugUI::Get().showCamIndicators  = !anyVisible;

    if (debug.gridPlane) debug.gridPlane->SetVisible(debug.showGrid);
}

void TestGameScene::Render()
{
    auto& sel = SceneDebugUI::Get().GetSelection();
    void* selPtr = (sel.type == SceneDebugUI::SelectionType::GameObject) ? sel.ptr : nullptr;
    GraphicsContext::GetInstance().GetCommandQueue()->SetWireframeSelectedObject(selPtr);

    ZNScene::Render();
}

void TestGameScene::RenderForward()
{
    ZNScene::RenderForward();

    SceneDebugUI::Get().onOutlinerExtras = [this]() {
        auto& sel = SceneDebugUI::Get().GetSelection();
        if (ImGui::TreeNodeEx("Cameras", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool isSelected = (sel.type == SceneDebugUI::SelectionType::Custom
                               && sel.ptr == cctv.camera);
            if (ImGui::Selectable("CCTV Camera", isSelected))
            {
                sel.type = SceneDebugUI::SelectionType::Custom;
                sel.ptr  = cctv.camera;
            }
            ImGui::TreePop();
        }
    };

    SceneDebugUI::Get().onInspectorExtras = [this](void* ptr) {
        ZNCamera* cam = static_cast<ZNCamera*>(ptr);
        ImGui::Text("Camera: CCTV");
        ImGui::Separator();
        ImGui::Text("Transform");
        ZNVector3 camPos = cam->GetPosition();
        float posArr[3] = { camPos.x, camPos.y, camPos.z };
        if (ImGui::DragFloat3("Position", posArr, 0.05f))
            cam->SetPosition(ZNVector3(posArr[0], posArr[1], posArr[2]));
        const float RAD_TO_DEG = 180.0f / 3.14159265f;
        float pitchDeg = cam->GetPitch() * RAD_TO_DEG;
        float yawDeg   = cam->GetYaw()   * RAD_TO_DEG;
        float rot[2]   = { pitchDeg, yawDeg };
        if (ImGui::DragFloat2("Pitch / Yaw", rot, 0.5f, -180.0f, 180.0f))
            cam->SetRotation(rot[0], rot[1]);
    };

    // Scene-specific Debug panel: Grid toggle only (spot/cam indicators handled by SceneDebugUI)
    SceneDebugUI::Get().onDebugExtras = [this]() {
        if (ImGui::Checkbox("Grid", &debug.showGrid))
            if (debug.gridPlane) debug.gridPlane->SetVisible(debug.showGrid);
    };
}
