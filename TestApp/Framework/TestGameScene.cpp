#include "TestGameScene.h"
#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <imgui.h>
#include "ZNFramework/Graphics/Platform/Direct3D12/RenderTexture.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/CommandQueue.h"
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
    this->dirLight = dirLight;

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
                obj->GetTransform().scale = ZNVector3(0.01f, 0.01f, 0.01f);

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

    // Forward-unlit shader for the CCTV pass (single RT, no GBuffer MRT)
    cctv.fwdShader = Platform::CreateShader();
    cctv.fwdShader->Load(GetResourcePath() / L"Shaders" / L"forward_unlit.hlsli");

    // Simple flat-colour materials matching scene object colours
    cctv.floorMat  = ZNMaterialFactory::CreatePBR(cctv.fwdShader, ZNVector4(0.6f, 0.6f, 0.6f, 1.0f), 0.0f, 1.0f);
    cctv.cubeMat   = ZNMaterialFactory::CreatePBR(cctv.fwdShader, ZNVector4(0.2f, 0.4f, 0.9f, 1.0f), 0.0f, 1.0f);
    cctv.sphereMat = ZNMaterialFactory::CreatePBR(cctv.fwdShader, ZNVector4(0.9f, 0.7f, 0.1f, 1.0f), 0.0f, 1.0f);
    cctv.bunnyMat  = ZNMaterialFactory::CreatePBR(cctv.fwdShader, ZNVector4(0.8f, 0.1f, 0.1f, 1.0f), 0.0f, 1.0f);

    // TV object: deferred material with CCTV RT as albedo
    cctv.tvMat = ZNMaterialFactory::CreatePBR(defaultShader, ZNVector4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 1.0f);
    static_cast<Material*>(cctv.tvMat)->SetAlbedoSRVHandle(cctv.rt->GetSRVCpuHandle());

    cctv.tvObj = new ZNGameObject();
    cctv.tvObj->SetMesh(ZNMeshFactory::CreatePlane(0.5f)); // unit-plane, scaled by transform
    cctv.tvObj->GetMesh()->SetMaterial(cctv.tvMat);
    cctv.tvObj->SetMaterial(cctv.tvMat);
    cctv.tvObj->SetName("TV");
    cctv.tvObj->SetTag("TV");
    cctv.tvObj->GetTransform().position = ZNVector3(-1.0f, 2.0f, 4.0f);
    cctv.tvObj->GetTransform().scale    = ZNVector3(3.2f, 1.8f, 0.1f); // 16:9 aspect
    cctv.tvObj->GetTransform().rotation = ZNVector3(-90.0f, 0.0f, 0.0f); // face -Z (toward cam)
    cctv.tvObj->SetCastShadow(false);
    AddGameObject(cctv.tvObj);

    // Register the offscreen pass — renders before the main GBuffer pass
    CommandQueue* cmdQ = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    cmdQ->AddOffscreenCamera(cctv.camera, cctv.rt, "CCTV",
        [this]() {
            // Render scene objects with forward-unlit materials (skip TV to avoid recursion)
            auto renderWith = [](ZNGameObject* obj, ZNMaterial* mat) {
                if (!obj || !obj->IsVisible() || !obj->GetMesh()) return;
                ZNMaterial* orig = obj->GetMaterial();
                obj->GetMesh()->SetMaterial(mat);
                obj->Render();
                obj->GetMesh()->SetMaterial(orig);
            };
            renderWith(scene.floor,  cctv.floorMat);
            renderWith(scene.cube,   cctv.cubeMat);
            renderWith(scene.sphere, cctv.sphereMat);
            for (auto* obj : models.objects)
                renderWith(obj, cctv.bunnyMat);
        });

    std::cout << "Scene initialized. Press F1 to toggle debug visuals." << std::endl;
}

void TestGameScene::Update(float deltaTime)
{
    ZNScene::Update(deltaTime);

    if (turntableObj && turntableEnabled)
    {
        turntableObj->GetTransform().rotation.y += 45.0f * deltaTime;
    }

    fpsAccum += deltaTime;
    fpsFrames++;
    if (fpsAccum >= 0.5f)
    {
        fpsDisplay = fpsFrames / fpsAccum;
        fpsAccum  = 0.0f;
        fpsFrames = 0;

        // CPU usage: system-wide via GetSystemTimes, sampled on the same 0.5s cadence
        static ULONGLONG prevIdle = 0, prevKernel = 0, prevUser = 0;
        FILETIME idle, kernel, user;
        if (GetSystemTimes(&idle, &kernel, &user))
        {
            auto toU64 = [](const FILETIME& ft) -> ULONGLONG {
                return (ULONGLONG(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
            };
            ULONGLONG idleU   = toU64(idle);
            ULONGLONG kernelU = toU64(kernel);
            ULONGLONG userU   = toU64(user);
            ULONGLONG idleDelta   = idleU   - prevIdle;
            ULONGLONG totalDelta  = (kernelU - prevKernel) + (userU - prevUser);
            if (totalDelta > 0)
                cpuUsagePercent = (1.0f - static_cast<float>(idleDelta) / totalDelta) * 100.0f;
            prevIdle   = idleU;
            prevKernel = kernelU;
            prevUser   = userU;
        }
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
    bool anyVisible = debug.showGrid || debug.showSpotLights;
    debug.showGrid       = !anyVisible;
    debug.showSpotLights = !anyVisible;

    if (debug.gridPlane) debug.gridPlane->SetVisible(debug.showGrid);
    SetSpotLightDebugVisible(debug.spotLight1, debug.showSpotLights);
    SetSpotLightDebugVisible(debug.spotLight2, debug.showSpotLights);
}

void TestGameScene::Render()
{
    // Update selected object for wireframe highlight before any geometry renders.
    void* selPtr = (selection.type == SelectedType::GameObject) ? selection.ptr : nullptr;
    GraphicsContext::GetInstance().GetCommandQueue()->SetWireframeSelectedObject(selPtr);

    ZNScene::Render();
}

void TestGameScene::RenderForward()
{
    ZNScene::RenderForward();

    // --- Stats ---
    const float cpuMs = fpsDisplay > 0.0f ? 1000.0f / fpsDisplay : 0.0f;
    const float gpuMs = GraphicsContext::GetInstance().GetCommandQueue()
                      ? GraphicsContext::GetInstance().GetCommandQueue()->GetGpuFrameTimeMs()
                      : 0.0f;
    const float gpuMemUsedMB   = GraphicsContext::GetInstance().GetDevice()
                               ? GraphicsContext::GetInstance().GetDevice()->GetGpuMemoryUsageMB()
                               : 0.0f;
    const float gpuMemBudgetMB = GraphicsContext::GetInstance().GetDevice()
                               ? GraphicsContext::GetInstance().GetDevice()->GetGpuMemoryBudgetMB()
                               : 0.0f;
    const int   totalObjs   = static_cast<int>(GetGameObjects().size());
    const int   visibleObjs = [&]{ int n=0; for (auto* o : GetGameObjects()) if (o->IsVisible()) ++n; return n; }();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(220, 0), ImGuiCond_Always);
    ImGui::Begin("Stats", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("FPS        %.1f", fpsDisplay);
    ImGui::Text("CPU        %.2f ms", cpuMs);
    ImGui::Text("CPU Use    %.1f%%", cpuUsagePercent);
    ImGui::Text("GPU        %.2f ms", gpuMs);
    ImGui::Separator();
    ImGui::Text("Draw Calls %d", ZNGameObject::GetLastFrameDrawCalls());
    ImGui::Text("Objects    %d / %d", visibleObjs, totalObjs);
    ImGui::Separator();
    ImGui::Text("GPU Mem    %.0f / %.0f MB", gpuMemUsedMB, gpuMemBudgetMB);
    ImGui::Text("Triangles  %d", ZNGameObject::GetLastFrameTriangles());
    ImGui::Text("Vertices   %d", ZNGameObject::GetLastFrameVertices());

    ImGui::End();

    // --- Debug ---
    ImGui::SetNextWindowSize(ImVec2(220, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug");

    if (ImGui::Checkbox("Grid", &debug.showGrid))
        if (debug.gridPlane) debug.gridPlane->SetVisible(debug.showGrid);

    if (ImGui::Checkbox("Spot Light Indicators", &debug.showSpotLights))
    {
        SetSpotLightDebugVisible(debug.spotLight1, debug.showSpotLights);
        SetSpotLightDebugVisible(debug.spotLight2, debug.showSpotLights);
    }

    ImGui::Separator();
    ImGui::Text("View Mode");
    ZNCommandQueue* cq = GraphicsContext::GetInstance().GetCommandQueue();
    bool wireframe = (cq->GetViewMode() == ViewMode::Wireframe);
    if (ImGui::Checkbox("Wireframe", &wireframe))
        cq->SetViewMode(wireframe ? ViewMode::Wireframe : ViewMode::Lit);

    ImGui::End();

    // --- Outliner ---
    ImGui::SetNextWindowSize(ImVec2(220, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Outliner");

    if (ImGui::TreeNodeEx("GameObjects", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (auto* obj : GetGameObjects())
        {
            if (obj->GetTag() == "Debug") continue;
            bool isSelected = (selection.type == SelectedType::GameObject && selection.ptr == obj);
            if (ImGui::Selectable(obj->GetName().c_str(), isSelected))
            {
                selection.type = SelectedType::GameObject;
                selection.ptr  = obj;
            }
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const auto& spots = GetSpotLights();
        for (int i = 0; i < (int)spots.size(); ++i)
        {
            std::string label = "SpotLight " + std::to_string(i);
            bool isSelected = (selection.type == SelectedType::SpotLight && selection.ptr == spots[i]);
            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                selection.type = SelectedType::SpotLight;
                selection.ptr  = spots[i];
            }
        }
        ZNDirectionalLight* dl = GetDirectionalLight();
        if (dl)
        {
            bool isSelected = (selection.type == SelectedType::DirectionalLight && selection.ptr == dl);
            if (ImGui::Selectable("DirectionalLight", isSelected))
            {
                selection.type = SelectedType::DirectionalLight;
                selection.ptr  = dl;
            }
        }
        ImGui::TreePop();
    }

    ImGui::End();

    // --- Inspector ---
    ImGui::SetNextWindowSize(ImVec2(280, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector");

    switch (selection.type)
    {
    case SelectedType::GameObject:
    {
        ZNGameObject* obj = static_cast<ZNGameObject*>(selection.ptr);
        ImGui::Text("GameObject: %s", obj->GetName().c_str());
        ImGui::Separator();

        ImGui::Text("Transform");
        Transform& t = obj->GetTransform();
        float pos[3] = { t.position.x, t.position.y, t.position.z };
        if (ImGui::DragFloat3("Position", pos, 0.01f))
            t.position = ZNVector3(pos[0], pos[1], pos[2]);
        float rot[3] = { t.rotation.x, t.rotation.y, t.rotation.z };
        if (ImGui::DragFloat3("Rotation", rot, 0.5f))
            t.rotation = ZNVector3(rot[0], rot[1], rot[2]);
        float sc[3] = { t.scale.x, t.scale.y, t.scale.z };
        if (ImGui::DragFloat3("Scale", sc, 0.001f, 0.001f, 100.0f))
            t.scale = ZNVector3(sc[0], sc[1], sc[2]);

        ZNMaterial* mat = obj->GetMaterial();
        if (mat)
        {
            ImGui::Separator();
            ImGui::Text("Material");
            MaterialParams p = mat->GetParams();
            bool changed = false;
            float col[4] = { p.albedoColor.x, p.albedoColor.y, p.albedoColor.z, p.albedoColor.w };
            if (ImGui::ColorEdit4("Albedo", col))
            {
                p.albedoColor = ZNVector4(col[0], col[1], col[2], col[3]);
                changed = true;
            }
            if (ImGui::SliderFloat("Metallic",  &p.metallic,  0.0f, 1.0f)) changed = true;
            if (ImGui::SliderFloat("Roughness", &p.roughness, 0.0f, 1.0f)) changed = true;
            if (changed)
                mat->SetParams(p);
        }
        break;
    }
    case SelectedType::SpotLight:
    {
        ZNSpotLight* light = static_cast<ZNSpotLight*>(selection.ptr);
        ImGui::Text("SpotLight");
        ImGui::Separator();

        ZNVector3 col = light->GetColor();
        float color[3] = { col.x, col.y, col.z };
        if (ImGui::ColorEdit3("Color", color))
            light->SetColor(ZNVector3(color[0], color[1], color[2]));

        float intensity = light->GetIntensity();
        if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f))
            light->SetIntensity(intensity);

        ZNVector3 pos = light->GetPosition();
        float posArr[3] = { pos.x, pos.y, pos.z };
        if (ImGui::DragFloat3("Position", posArr, 0.05f))
            light->SetPosition(ZNVector3(posArr[0], posArr[1], posArr[2]));

        float innerAngle = light->GetInnerCutoffAngle();
        float outerAngle = light->GetOuterCutoffAngle();
        bool cutoffChanged = false;
        if (ImGui::SliderFloat("Inner Angle", &innerAngle, 0.0f, 89.0f)) cutoffChanged = true;
        if (ImGui::SliderFloat("Outer Angle", &outerAngle, 0.0f, 89.0f)) cutoffChanged = true;
        if (cutoffChanged)
            light->SetCutoffAngle(innerAngle, outerAngle);
        break;
    }
    case SelectedType::DirectionalLight:
    {
        ZNDirectionalLight* dl = static_cast<ZNDirectionalLight*>(selection.ptr);
        ImGui::Text("DirectionalLight");
        ImGui::Separator();

        ZNVector3 col = dl->GetColor();
        float color[3] = { col.x, col.y, col.z };
        if (ImGui::ColorEdit3("Color", color))
            dl->SetColor(ZNVector3(color[0], color[1], color[2]));

        float intensity = dl->GetIntensity();
        if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 20.0f))
            dl->SetIntensity(intensity);

        ZNVector3 dir = dl->GetDirection();
        float dirArr[3] = { dir.x, dir.y, dir.z };
        if (ImGui::SliderFloat3("Direction", dirArr, -1.0f, 1.0f))
            dl->SetDirection(ZNVector3(dirArr[0], dirArr[1], dirArr[2]).Normalize());
        break;
    }
    default:
        ImGui::TextDisabled("Nothing selected.");
        break;
    }

    ImGui::End();
}
