#include "CCTVScene.h"
#include "SceneDebugUI.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include "ZNFramework/Graphics/Platform/Direct3D12/RenderTexture.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/CommandQueue.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/Material.h"

using namespace ZNFramework;

// Layout (top-down, +Z forward):
//   Player cam  @ (0, 3, -6)  looking +Z
//   Monitored objects near  Z = 1-2
//   TV screen   @ (0, 2.5, 4) facing player (-Z normal)
//   CCTV cam    @ (0, 7, 1.5) pitched -80deg (nearly overhead)

void CCTVScene::Initialize()
{
    defaultShader = Platform::CreateShader();
    defaultShader->Load(GetResourcePath() / L"Shaders" / L"deferred_lighting.hlsli");

    cctvShader = Platform::CreateShader();
    cctvShader->Load(GetResourcePath() / L"Shaders" / L"forward_lit.hlsli");

    // --- Player camera ---
    ZNCamera* cam = new ZNCamera();
    cam->SetPosition(ZNVector3(0.0f, 3.0f, -6.0f));
    cam->SetRotation(-10.0f, 0.0f);
    cam->SetMoveSpeed(3.0f);
    SetCamera(cam);

    // --- Directional light ---
    ZNDirectionalLight* dirLight = Platform::CreateDirectionalLight();
    dirLight->SetDirection(ZNVector3(0.3f, -1.0f, 0.5f));
    dirLight->SetIntensity(5.0f);
    dirLight->SetColor(ZNVector3(0.6f, 0.6f, 0.6f));
    dirLight->SetAmbientIntensity(0.8f);
    dirLight->SetShadowFocusPoint(ZNVector3(0.0f, 0.0f, 2.0f));
    dirLight->SetShadowBounds(20.0f, 0.1f, 50.0f);
    SetDirectionalLight(dirLight);

    // --- Floor ---
    floorMat = ZNMaterialFactory::CreatePBR(defaultShader,
        ZNVector4(0.45f, 0.45f, 0.45f, 1.0f), 0.0f, 0.9f);
    floor = new ZNGameObject();
    floor->SetMesh(ZNMeshFactory::CreatePlane(8.0f));
    floor->GetMesh()->SetMaterial(floorMat);
    floor->SetMaterial(floorMat);
    floor->SetName("Floor");
    floor->SetCastShadow(false);
    AddGameObject(floor);

    // --- Monitored objects ---
    boxAMat = ZNMaterialFactory::CreatePBR(defaultShader,
        ZNVector4(0.9f, 0.2f, 0.2f, 1.0f), 0.0f, 0.5f); // red
    boxA = new ZNGameObject();
    boxA->SetMesh(ZNMeshFactory::CreateCube(0.6f));
    boxA->GetMesh()->SetMaterial(boxAMat);
    boxA->SetMaterial(boxAMat);
    boxA->SetName("Box A");
    boxA->GetTransform().position = ZNVector3(-2.0f, 0.3f, 1.5f);
    boxA->GetTransform().scale    = ZNVector3(0.5f, 0.5f, 0.5f);
    AddGameObject(boxA);

    boxBMat = ZNMaterialFactory::CreatePBR(defaultShader,
        ZNVector4(0.2f, 0.5f, 0.9f, 1.0f), 0.0f, 0.5f); // blue
    boxB = new ZNGameObject();
    boxB->SetMesh(ZNMeshFactory::CreateCube(0.6f));
    boxB->GetMesh()->SetMaterial(boxBMat);
    boxB->SetMaterial(boxBMat);
    boxB->SetName("Box B");
    boxB->GetTransform().position = ZNVector3(0.0f, 0.4f, 2.0f);
    boxB->GetTransform().scale    = ZNVector3(0.7f, 0.7f, 0.7f);
    boxB->GetTransform().rotation = ZNVector3(0.0f, 30.0f, 0.0f);
    AddGameObject(boxB);

    boxCMat = ZNMaterialFactory::CreatePBR(defaultShader,
        ZNVector4(0.2f, 0.85f, 0.3f, 1.0f), 0.0f, 0.5f); // green
    boxC = new ZNGameObject();
    boxC->SetMesh(ZNMeshFactory::CreateCube(0.6f));
    boxC->GetMesh()->SetMaterial(boxCMat);
    boxC->SetMaterial(boxCMat);
    boxC->SetName("Box C");
    boxC->GetTransform().position = ZNVector3(2.0f, 0.3f, 1.0f);
    boxC->GetTransform().scale    = ZNVector3(0.5f, 0.5f, 0.5f);
    AddGameObject(boxC);

    sphereMat = ZNMaterialFactory::CreatePBR(defaultShader,
        ZNVector4(1.0f, 0.85f, 0.1f, 1.0f), 0.7f, 0.2f); // gold metallic
    sphere = new ZNGameObject();
    sphere->SetMesh(ZNMeshFactory::CreateSphere(0.4f, 16, 16));
    sphere->GetMesh()->SetMaterial(sphereMat);
    sphere->SetMaterial(sphereMat);
    sphere->SetName("Sphere");
    sphere->GetTransform().position = ZNVector3(0.5f, 0.4f, 0.5f);
    AddGameObject(sphere);

    // --- CCTV infrastructure ---
    cctvRT = new RenderTexture();
    cctvRT->Init(512, 288);

    // Overhead camera: positioned high, pitching sharply down
    cctvCamera = new ZNCamera();
    cctvCamera->SetPosition(ZNVector3(0.0f, 7.0f, 1.5f));
    cctvCamera->SetRotation(-80.0f, 0.0f); // nearly straight down
    cctvCamera->SetPerspective(3.141592f / 3.0f, 512.0f / 288.0f, 0.1f, 50.0f); // 60deg wide

    // Forward-lit materials (same colours as main-pass materials but forward shader)
    cctvFloorMat  = ZNMaterialFactory::CreatePBR(cctvShader, ZNVector4(0.45f, 0.45f, 0.45f, 1.0f), 0.0f, 0.9f);
    cctvBoxAMat   = ZNMaterialFactory::CreatePBR(cctvShader, ZNVector4(0.9f,  0.2f,  0.2f,  1.0f), 0.0f, 0.5f);
    cctvBoxBMat   = ZNMaterialFactory::CreatePBR(cctvShader, ZNVector4(0.2f,  0.5f,  0.9f,  1.0f), 0.0f, 0.5f);
    cctvBoxCMat   = ZNMaterialFactory::CreatePBR(cctvShader, ZNVector4(0.2f,  0.85f, 0.3f,  1.0f), 0.0f, 0.5f);
    cctvSphereMat = ZNMaterialFactory::CreatePBR(cctvShader, ZNVector4(1.0f,  0.85f, 0.1f,  1.0f), 0.0f, 0.5f);

    // TV screen: CreatePlane is XZ; after -90°X rotation X=width, Z(local)=height(world)
    tvMat = ZNMaterialFactory::CreatePBR(defaultShader,
        ZNVector4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 1.0f);
    static_cast<Material*>(tvMat)->SetAlbedoSRVHandle(cctvRT->GetSRVCpuHandle());

    tvScreen = new ZNGameObject();
    tvScreen->SetMesh(ZNMeshFactory::CreatePlane(0.5f));
    tvScreen->GetMesh()->SetMaterial(tvMat);
    tvScreen->SetMaterial(tvMat);
    tvScreen->SetName("TV Screen");
    tvScreen->SetTag("TV");
    tvScreen->GetTransform().position = ZNVector3(0.0f, 2.5f, 4.0f);
    tvScreen->GetTransform().scale    = ZNVector3(4.0f, 0.05f, 2.25f); // 16:9
    tvScreen->GetTransform().rotation = ZNVector3(-90.0f, 0.0f, 0.0f);
    tvScreen->SetCastShadow(false);
    AddGameObject(tvScreen);

    // --- Room model (FBX) ---
    {
        std::filesystem::path roomPath =
            GetResourcePath() / L"Models" / L"33-the-room-2" / L"The room" / L"room.fbx";

        if (std::filesystem::exists(roomPath))
        {
            std::cout << "[CCTVScene] Loading room.fbx..." << std::endl;
            ZNModelLoader* loader = Platform::CreateModelLoader();
            ModelData modelData;
            if (loader->Load(roomPath, modelData))
            {
                // One material per material slot from FBX; clamp near-black albedo to visible
                for (const auto& matData : modelData.materials)
                {
                    ZNVector4 albedo = matData.params.albedoColor;
                    float lum = albedo.x * 0.299f + albedo.y * 0.587f + albedo.z * 0.114f;
                    if (lum < 0.05f)
                        albedo = ZNVector4(0.8f, 0.75f, 0.70f, 1.0f); // fallback warm-white
                    float roughness = (matData.params.roughness > 0.25f) ? matData.params.roughness : 0.25f;
                    room.materials.push_back(ZNMaterialFactory::CreatePBR(
                        defaultShader, albedo, matData.params.metallic, roughness));
                }
                if (room.materials.empty())
                    room.materials.push_back(ZNMaterialFactory::CreatePBR(
                        defaultShader, ZNVector4(0.8f, 0.75f, 0.70f, 1.0f), 0.0f, 0.6f));

                // Single warm-white material for the CCTV offscreen pass
                room.cctvMat = ZNMaterialFactory::CreatePBR(
                    cctvShader, ZNVector4(0.8f, 0.75f, 0.70f, 1.0f), 0.0f, 0.6f);

                for (const auto& meshData : modelData.meshes)
                {
                    size_t matIdx = (meshData.materialIndex < room.materials.size())
                                  ? meshData.materialIndex : 0;
                    ZNMaterial* mat = room.materials[matIdx];

                    ZNMesh* mesh = Platform::CreateMesh();
                    mesh->Init(meshData.vertices, meshData.indices);
                    mesh->SetMaterial(mat);

                    ZNGameObject* obj = new ZNGameObject();
                    obj->SetMesh(mesh);
                    obj->SetMaterial(mat);
                    obj->SetName("Room_" + std::to_string(room.objects.size()));
                    obj->GetTransform().scale = ZNVector3(0.01f, 0.01f, 0.01f);
                    obj->SetCastShadow(false);
                    AddGameObject(obj);
                    room.objects.push_back(obj);
                }
                std::cout << "[CCTVScene] Room loaded: " << room.objects.size()
                          << " meshes, " << room.materials.size() << " materials." << std::endl;
            }
            else
            {
                std::cout << "[CCTVScene] Failed to load room.fbx." << std::endl;
            }
            delete loader;
        }
        else
        {
            std::cout << "[CCTVScene] room.fbx not found at: " << roomPath << std::endl;
        }
    }

    // Register offscreen pass (unique name to avoid conflict with TestGameScene's "CCTV")
    CommandQueue* cmdQ = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    cmdQ->AddOffscreenCamera(cctvCamera, cctvRT, "CCTV_Room",
        [this]() {
            auto renderWith = [](ZNGameObject* obj, ZNMaterial* mat) {
                if (!obj || !obj->IsVisible() || !obj->GetMesh()) return;
                ZNMaterial* orig = obj->GetMaterial();
                obj->GetMesh()->SetMaterial(mat);
                obj->Render();
                obj->GetMesh()->SetMaterial(orig);
            };
            // Room model (all meshes use a single warm-white forward-lit mat in CCTV view)
            if (room.cctvMat)
            {
                for (auto* obj : room.objects)
                    renderWith(obj, room.cctvMat);
            }
            renderWith(floor,  cctvFloorMat);
            renderWith(boxA,   cctvBoxAMat);
            renderWith(boxB,   cctvBoxBMat);
            renderWith(boxC,   cctvBoxCMat);
            renderWith(sphere, cctvSphereMat);
        });
}

void CCTVScene::Render()
{
    ZNScene::Render();
}

void CCTVScene::RenderForward()
{
    ZNScene::RenderForward();

    SceneDebugUI::Get().onInspectorExtras = nullptr;

    // Inject room meshes as a collapsible Outliner section so the main GameObjects
    // list stays readable while the room's 100+ pieces remain accessible.
    if (!room.objects.empty())
    {
        SceneDebugUI::Get().onOutlinerExtras = [this]() {
            std::string label = "Room Model (" + std::to_string(room.objects.size()) + ")";
            if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_None))
            {
                auto& sel = SceneDebugUI::Get().GetSelection();
                for (auto* obj : room.objects)
                {
                    bool isSel = (sel.type == SceneDebugUI::SelectionType::GameObject && sel.ptr == obj);
                    if (ImGui::Selectable(obj->GetName().c_str(), isSel))
                    {
                        sel.type = SceneDebugUI::SelectionType::GameObject;
                        sel.ptr  = obj;
                    }
                }
                ImGui::TreePop();
            }
        };
    }
    else
    {
        SceneDebugUI::Get().onOutlinerExtras = nullptr;
    }
}
