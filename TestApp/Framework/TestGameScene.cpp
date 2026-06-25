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

namespace
{
    void SetupSpotLightDebug(
        TestGameScene::SpotLightDebug& debug,
        ZNSpotLight* spotLight,
        const ZNVector4& color,
        ZNShader* defaultShader,
        ZNShader* coneShader,
        float coneLength,
        ZNScene* scene)
    {
        ZNVector3 pos = spotLight->GetPosition();
        ZNVector3 dir = spotLight->GetDirection();
        float outerAngle = spotLight->GetOuterCutoffAngle();

        // Marker (solid cube)
        debug.markerMaterial = ZNMaterialFactory::CreatePBR(defaultShader,
            ZNVector4(color.x, color.y, color.z, 1.0f), 0.0f, 1.0f);
        debug.marker = new ZNGameObject();
        debug.marker->SetMesh(ZNMeshFactory::CreateCube(0.1f));
        debug.marker->GetMesh()->SetMaterial(debug.markerMaterial);
        debug.marker->SetName("SpotLightMarker");
        debug.marker->SetTag("Debug");
        debug.marker->GetTransform().position = pos;
        debug.marker->SetVisible(false);
        scene->AddGameObject(debug.marker);

        // Cone (transparent, no depth test)
        debug.coneMaterial = ZNMaterialFactory::CreatePBR(coneShader,
            ZNVector4(color.x, color.y, color.z, 0.2f), 0.0f, 1.0f);
        debug.cone = new ZNGameObject();
        debug.cone->SetMesh(ZNMeshFactory::CreateConeFromApex(outerAngle, coneLength, 32));
        debug.cone->GetMesh()->SetMaterial(debug.coneMaterial);
        debug.cone->SetName("SpotLightCone");
        debug.cone->SetTag("Debug");
        debug.cone->GetTransform().position = pos;
        float zRotation = atan2f(-dir.x, -dir.y) * 180.0f / 3.14159265f;
        float xRotation = asinf(dir.z) * 180.0f / 3.14159265f;
        debug.cone->GetTransform().rotation = ZNVector3(xRotation, 0.0f, zRotation);
        debug.cone->SetVisible(false);
        debug.cone->SetCastShadow(false);
        scene->AddForwardGameObject(debug.cone);
    }

    void SetSpotLightDebugVisible(TestGameScene::SpotLightDebug& debug, bool visible)
    {
        if (debug.marker) debug.marker->SetVisible(visible);
        if (debug.cone) debug.cone->SetVisible(visible);
    }
}

void TestGameScene::Initialize()
{
    // Load shaders
    defaultShader = Platform::CreateShader();
    defaultShader->Load(GetResourcePath() / L"Shaders" / L"deferred_lighting.hlsli");

    gridShader = Platform::CreateShader();
    gridShader->Load(GetResourcePath() / L"Shaders" / L"grid.hlsli");
    gridShader->EnableAlphaBlend();

    debugConeShader = Platform::CreateShader();
    debugConeShader->Load(GetResourcePath() / L"Shaders" / L"forward_unlit.hlsli");
    debugConeShader->EnableAlphaBlend();
    debugConeShader->DisableDepthWrite();

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

    // Spot light 2 (Cyan)
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

    // Debug: Spotlight 1 (Green)
    SetupSpotLightDebug(debug.spotLight1, spotLight1, ZNVector4(0.0f, 1.0f, 0.0f, 1.0f),
        defaultShader, debugConeShader, 4.0f, this);

    // Debug: Spotlight 2 (Red)
    SetupSpotLightDebug(debug.spotLight2, spotLight2, ZNVector4(1.0f, 0.0f, 0.0f, 1.0f),
        defaultShader, debugConeShader, 4.0f, this);

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
    // Render texture for the CCTV feed (16:9)
    cctv.rt = new RenderTexture();
    cctv.rt->Init(512, 288);

    // Offscreen camera: side-angle view of the scene
    cctv.camera = new ZNCamera();
    cctv.camera->SetPosition(ZNVector3(6.0f, 4.0f, 0.0f));
    cctv.camera->SetRotation(-25.0f, -100.0f);
    cctv.camera->SetPerspective(3.141592f / 4.0f, 512.0f / 288.0f, 0.1f, 100.0f);

    // Forward PBR shader for the CCTV offscreen pass
    cctv.fwdShader = Platform::CreateShader();
    cctv.fwdShader->Load(GetResourcePath() / L"Shaders" / L"forward_pbr.hlsli");

    // Unlit textured shader for the TV screen object (no deferred lighting re-applied)
    cctv.tvUnlitShader = Platform::CreateShader();
    cctv.tvUnlitShader->Load(GetResourcePath() / L"Shaders" / L"screen_unlit.hlsli");

    // TV object: unlit material — samples CCTV RT directly without deferred lighting
    cctv.tvMat = ZNMaterialFactory::CreatePBR(cctv.tvUnlitShader, ZNVector4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 1.0f);
    static_cast<Material*>(cctv.tvMat)->SetAlbedoSRVHandle(cctv.rt->GetSRVCpuHandle());

    cctv.tvObj = new ZNGameObject();
    cctv.tvObj->SetMesh(ZNMeshFactory::CreatePlane(0.5f)); // unit-plane, scaled by transform
    cctv.tvObj->GetMesh()->SetMaterial(cctv.tvMat);
    cctv.tvObj->SetMaterial(cctv.tvMat);
    cctv.tvObj->SetName("TV");
    cctv.tvObj->SetTag("TV");
    cctv.tvObj->GetTransform().position = ZNVector3(-1.0f, 2.0f, 4.0f);
    cctv.tvObj->GetTransform().scale    = ZNVector3(3.2f, 0.1f, 1.8f); // X=width, Z=height (after -90°X rot)
    cctv.tvObj->GetTransform().rotation = ZNVector3(-90.0f, 0.0f, 0.0f); // face -Z (toward cam)
    cctv.tvObj->SetCastShadow(false);
    AddForwardGameObject(cctv.tvObj);

    // Auto-render: all scene gameObjects use their own material params through fwdShader.
    AddOffscreenCamera(cctv.camera, cctv.rt, "CCTV", cctv.fwdShader);

    // Debug: CCTV camera position/direction indicator
    debug.cameraMarkerMaterial = ZNMaterialFactory::CreatePBR(defaultShader,
        ZNVector4(0.0f, 0.9f, 1.0f, 1.0f), 0.0f, 0.5f); // cyan

    debug.cameraMarker = new ZNGameObject();
    debug.cameraMarker->SetMesh(ZNMeshFactory::CreateCube(0.1f));
    debug.cameraMarker->GetMesh()->SetMaterial(debug.cameraMarkerMaterial);
    debug.cameraMarker->SetName("CCTVCameraMarker");
    debug.cameraMarker->SetTag("Debug");
    debug.cameraMarker->GetTransform().position = cctv.camera->GetPosition();
    debug.cameraMarker->SetVisible(false);
    debug.cameraMarker->SetCastShadow(false);
    AddGameObject(debug.cameraMarker);

    debug.cameraLens = new ZNGameObject();
    debug.cameraLens->SetMesh(ZNMeshFactory::CreateCube(0.05f));
    debug.cameraLens->GetMesh()->SetMaterial(debug.cameraMarkerMaterial);
    debug.cameraLens->SetName("CCTVCameraLens");
    debug.cameraLens->SetTag("Debug");
    debug.cameraLens->GetTransform().position =
        cctv.camera->GetPosition() + cctv.camera->GetForward() * 0.35f;
    debug.cameraLens->SetVisible(false);
    debug.cameraLens->SetCastShadow(false);
    AddGameObject(debug.cameraLens);

    std::cout << "Scene initialized. Press F1 to toggle debug visuals." << std::endl;
}

void TestGameScene::Update(float deltaTime)
{
    ZNScene::Update(deltaTime);

    if (turntableObj && turntableEnabled)
    {
        turntableObj->GetTransform().rotation.y += 45.0f * deltaTime;
    }

    if (debug.cameraMarker && debug.cameraMarker->IsVisible())
    {
        ZNVector3 camPos = cctv.camera->GetPosition();
        ZNVector3 camFwd = cctv.camera->GetForward();
        debug.cameraMarker->GetTransform().position = camPos;
        if (debug.cameraLens)
            debug.cameraLens->GetTransform().position = camPos + camFwd * 0.35f;
    }
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
    bool anyVisible = debug.showGrid || debug.showSpotLights || debug.showCameras;
    debug.showGrid       = !anyVisible;
    debug.showSpotLights = !anyVisible;
    debug.showCameras    = !anyVisible;

    if (debug.gridPlane) debug.gridPlane->SetVisible(debug.showGrid);
    SetSpotLightDebugVisible(debug.spotLight1, debug.showSpotLights);
    SetSpotLightDebugVisible(debug.spotLight2, debug.showSpotLights);
    if (debug.cameraMarker) debug.cameraMarker->SetVisible(debug.showCameras);
    if (debug.cameraLens)   debug.cameraLens->SetVisible(debug.showCameras);
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

    // Register scene-specific Outliner and Inspector extensions for this frame.
    // These lambdas are consumed by SceneDebugUI::Render(), which runs after this call.
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

    if (!SceneDebugUI::Get().IsVisible()) return;

    // --- Debug (scene-specific toggles) ---
    ImGui::SetNextWindowSize(ImVec2(220.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug");

    if (ImGui::Checkbox("Grid", &debug.showGrid))
        if (debug.gridPlane) debug.gridPlane->SetVisible(debug.showGrid);

    if (ImGui::Checkbox("Spot Light Indicators", &debug.showSpotLights))
    {
        SetSpotLightDebugVisible(debug.spotLight1, debug.showSpotLights);
        SetSpotLightDebugVisible(debug.spotLight2, debug.showSpotLights);
    }
    if (ImGui::Checkbox("Camera Indicators", &debug.showCameras))
    {
        if (debug.cameraMarker) debug.cameraMarker->SetVisible(debug.showCameras);
        if (debug.cameraLens)   debug.cameraLens->SetVisible(debug.showCameras);
    }

    ImGui::Separator();
    ImGui::Text("View Mode");
    ZNCommandQueue* cq = GraphicsContext::GetInstance().GetCommandQueue();
    bool wireframe = (cq->GetViewMode() == ViewMode::Wireframe);
    if (ImGui::Checkbox("Wireframe", &wireframe))
        cq->SetViewMode(wireframe ? ViewMode::Wireframe : ViewMode::Lit);

    ImGui::End();
}
