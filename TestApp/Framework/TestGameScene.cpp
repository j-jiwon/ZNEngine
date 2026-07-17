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

    // Camera — pulled back/up enough to frame the whole 8x5 sphere grid (see below)
    ZNCamera* cam = new ZNCamera();
    cam->SetPosition(ZNVector3(-0.030f, 1.367f, -14.822f));
    cam->SetRotation(-2.56f, -1.15f);
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
    dirLight->SetDirection(ZNVector3(-0.5f, -0.5f, 0.5f));
    dirLight->SetIntensity(1.5f);
    dirLight->SetColor(ZNVector3(1.0f, 1.0f, 1.0f));
    dirLight->SetAmbientIntensity(1.0f);
    dirLight->SetShadowFocusPoint(ZNVector3(0.0f, 0.0f, 0.0f));
    dirLight->SetShadowBounds(50.0f, 0.1f, 100.0f);
    SetDirectionalLight(dirLight);

    // Scene: PBR test grid — rows vary roughness (top=1.0 rough -> bottom=0.0 smooth),
    // columns vary metallic (left=0.0 non-metallic -> right=1.0 metallic). Same neutral
    // albedo across the whole grid so only roughness/metallic drive the look, same as
    // the reference chart. Needs the env cubemap (set below) for the IBL specular to
    // actually show anything on the smooth/metallic corner.
    const float kGridSpacing = 1.2f;
    const float kGridBaseY = 1.5f;
    {
        ZNVector4 gridAlbedo(0.9f, 0.9f, 0.95f, 1.0f);

        for (int row = 0; row < SceneObjects::kGridRows; ++row)
        {
            float roughness = 1.0f - static_cast<float>(row) / static_cast<float>(SceneObjects::kGridRows - 1);
            for (int col = 0; col < SceneObjects::kGridCols; ++col)
            {
                float metallic = static_cast<float>(col) / static_cast<float>(SceneObjects::kGridCols - 1);

                ZNMaterial* mat = ZNMaterialFactory::CreatePBR(defaultShader, gridAlbedo, metallic, roughness);
                scene.sphereMaterials.push_back(mat);

                ZNGameObject* obj = new ZNGameObject();
                obj->SetMesh(ZNMeshFactory::CreateSphere(1.0f, 16, 16));
                obj->GetMesh()->SetMaterial(mat);
                obj->SetMaterial(mat);
                obj->SetName("Sphere_r" + std::to_string(row) + "_c" + std::to_string(col));
                obj->GetTransform().position = ZNVector3(
                    (col - (SceneObjects::kGridCols - 1) / 2.0f) * kGridSpacing,
                    kGridBaseY - (row - (SceneObjects::kGridRows - 1) / 2.0f) * kGridSpacing,
                    0.0f);
                obj->GetTransform().scale = ZNVector3(0.5f, 0.5f, 0.5f);
                AddGameObject(obj);
                scene.spheres.push_back(obj);
            }
        }
    }

    // Env cubemap (static skybox) — feeds the sphere grid's IBL diffuse/specular so the
    // roughness/metallic variation is actually visible (see deferred_lighting.hlsli).
    SetEnvCubemapTexture(GetResourcePath() / L"Textures" / L"skybox_day.jpg");

    // Load bunny model — duplicated into a single row below the sphere grid, all
    // identical for now; reserved for a different (non-PBR-grid) test later.
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

            const int bunnyCount = (int)(SceneObjects::kGridCols * 0.5f);
            const float bunnySpacing = kGridSpacing * 2.0f;
            // One row below the sphere grid's bottom row, plus an extra gap.
            const float bottomRowY = kGridBaseY - (SceneObjects::kGridRows - 1) / 2.0f * kGridSpacing;
            const float bunnyY = bottomRowY - kGridSpacing * 1.5f;

            for (int i = 0; i < bunnyCount; ++i)
            {
                for (const auto& meshData : modelData.meshes)
                {
                    ZNGameObject* obj = new ZNGameObject();
                    ZNMesh* mesh = Platform::CreateMesh();
                    mesh->Init(meshData.vertices, meshData.indices);
                    mesh->SetMaterial(bunnyMat);

                    obj->SetMesh(mesh);
                    obj->SetMaterial(bunnyMat);
                    obj->SetName("Bunny_" + std::to_string(i));
                    obj->GetTransform().position = ZNVector3(
                        (i - (bunnyCount - 1) / 2.0f) * bunnySpacing, bunnyY, 0.0f);
                    obj->GetTransform().rotation = ZNVector3(0.f, 0.f, 0.f);
                    obj->GetTransform().scale = ZNVector3(0.00005f, 0.00005f, 0.00005f);

                    AddGameObject(obj);
                    models.objects.push_back(obj);
                }
            }

            if (!models.objects.empty())
                turntableObj = models.objects.front();
        }
        delete loader;
    }

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
    ZNLOG_INFO(LogChannel::Scene, "TestGameScene initialized (F1 toggles debug visuals)");
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
        ZNLOG_INFO(LogChannel::Scene, "Turntable: %s (%s)",
                   turntableEnabled ? "ON" : "OFF",
                   turntableObj ? turntableObj->GetName().c_str() : "-");
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

    // Scene-specific Debug panel: Grid toggle only (spot/cam indicators handled by SceneDebugUI)
    SceneDebugUI::Get().onDebugExtras = [this]() {
        if (ImGui::Checkbox("Grid", &debug.showGrid))
            if (debug.gridPlane) debug.gridPlane->SetVisible(debug.showGrid);
    };
}
