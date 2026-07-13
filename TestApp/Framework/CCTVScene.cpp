#include "CCTVScene.h"
#include "SceneDebugUI.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include "ZNFramework/Graphics/Platform/Direct3D12/RenderTexture.h"
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
    cctvShader->Load(GetResourcePath() / L"Shaders" / L"forward_pbr.hlsli");

    tvUnlitShader = Platform::CreateShader();
    tvUnlitShader->Load(GetResourcePath() / L"Shaders" / L"screen_unlit.hlsli");

    glassShader = Platform::CreateShader();
    glassShader->Load(GetResourcePath() / L"Shaders" / L"forward_lit.hlsli");
    glassShader->EnableAlphaBlend();
    glassShader->DisableDepthWrite();

    // --- Player camera --- same room.glb bounds as MirrorBallScene (X:[-1.43,1.43]
    // Y:[0.10,1.87] Z:[-1.55,1.75]), so reuse its camera placement.
    ZNCamera* cam = new ZNCamera();
    cam->SetPosition(ZNVector3(2.636f, 3.195f, -3.519f));
    cam->SetRotation(-20.63f, -41.90f);
    cam->SetMoveSpeed(1.5f);
    SetCamera(cam);

    // --- Directional light ---
    ZNDirectionalLight* dirLight = Platform::CreateDirectionalLight();
    dirLight->SetDirection(ZNVector3(0.3f, -1.0f, 0.5f));
    dirLight->SetIntensity(2.5f);
    dirLight->SetColor(ZNVector3(0.8f, 0.8f, 0.8f));
    dirLight->SetAmbientIntensity(0.8f);
    dirLight->SetShadowFocusPoint(ZNVector3(0.0f, 0.0f, 2.0f));
    dirLight->SetShadowBounds(20.0f, 0.1f, 50.0f);
    SetDirectionalLight(dirLight);

    // --- CCTV infrastructure ---
    cctvRT = new RenderTexture();
    cctvRT->Init(512, 288);

    // Overhead camera: positioned high, pitching sharply down
    cctvCamera = new ZNCamera();
    cctvCamera->SetPosition(ZNVector3(-1.15f, 1.8f, 1.25f));
    cctvCamera->SetRotation(-36.5f, 144.0f); // nearly straight down
    cctvCamera->SetPerspective(3.141592f / 3.0f, 512.0f / 288.0f, 0.1f, 50.0f); // 60deg wide

    // TV screen: CreatePlane is XZ; after -90°X rotation X=width, Z(local)=height(world)
    tvMat = ZNMaterialFactory::CreatePBR(tvUnlitShader,
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
    AddForwardGameObject(tvScreen);

    // Register CCTV camera for SceneDebugUI's common camera indicator
    RegisterDebugCamera(cctvCamera, "CCTV Overhead");

    // --- Room model (glTF binary, same room.glb + loading logic as MirrorBallScene) ---
    {
        std::filesystem::path roomPath =
            GetResourcePath() / L"Models" / L"room.glb";

        if (std::filesystem::exists(roomPath))
        {
            std::cout << "[CCTVScene] Loading room.glb..." << std::endl;
            ZNModelLoader* loader = Platform::CreateModelLoader();
            ModelData modelData;
            if (loader->Load(roomPath, modelData))
            {
                // One material per glTF material slot; textures (embedded or file-based) are
                // loaded and bound automatically. Flat-color fallback only kicks in when a
                // slot has no albedo texture and its baked color is near-black. Materials
                // with baseColor alpha < 1 (e.g. window glass) can't be represented by the
                // deferred G-buffer pass (opaque, no blending) - route them through
                // glassShader (forward + alpha blend) instead.
                std::vector<bool> roomMatIsTransparent;
                for (const auto& matData : modelData.materials)
                {
                    MaterialData patched = matData;
                    bool hasAlbedoTex =
                        !patched.texturePaths[static_cast<size_t>(TextureType::Albedo)].empty() ||
                        !patched.embeddedTextureData[static_cast<size_t>(TextureType::Albedo)].empty();
                    bool isTransparent = patched.params.albedoColor.w < 0.98f;

                    if (!hasAlbedoTex && !isTransparent)
                    {
                        ZNVector4& albedo = patched.params.albedoColor;
                        float lum = albedo.x * 0.299f + albedo.y * 0.587f + albedo.z * 0.114f;
                        if (lum < 0.05f)
                            albedo = ZNVector4(0.8f, 0.75f, 0.70f, 1.0f); // fallback warm-white
                        if (patched.params.roughness < 0.25f)
                            patched.params.roughness = 0.25f;
                    }

                    room.materials.push_back(ZNMaterialFactory::CreatePBRFromData(
                        isTransparent ? glassShader : defaultShader, patched));
                    roomMatIsTransparent.push_back(isTransparent);
                }
                if (room.materials.empty())
                {
                    MaterialData fallback;
                    fallback.params.albedoColor = ZNVector4(0.8f, 0.75f, 0.70f, 1.0f);
                    fallback.params.roughness   = 0.6f;
                    room.materials.push_back(ZNMaterialFactory::CreatePBRFromData(defaultShader, fallback));
                    roomMatIsTransparent.push_back(false);
                }

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
                    obj->SetTag("Room");
                    obj->SetCastShadow(false);

                    if (roomMatIsTransparent[matIdx])
                        AddForwardGameObject(obj);
                    else
                        AddGameObject(obj);
                    room.objects.push_back(obj);
                }
                std::cout << "[CCTVScene] Room loaded: " << room.objects.size()
                          << " meshes, " << room.materials.size() << " materials." << std::endl;
            }
            else
            {
                std::cout << "[CCTVScene] Failed to load room.glb." << std::endl;
            }
            delete loader;
        }
        else
        {
            std::cout << "[CCTVScene] room.glb not found at: " << roomPath << std::endl;
        }
    }

    // Auto-render: all scene gameObjects use their own material params through cctvShader.
    // No manual per-object CCTV material needed.
    AddOffscreenCamera(cctvCamera, cctvRT, "CCTV_Room", cctvShader);
}

void CCTVScene::Update(float deltaTime)
{
    ZNScene::Update(deltaTime);
}

void CCTVScene::Render()
{
    auto& sel = SceneDebugUI::Get().GetSelection();
    void* selPtr = (sel.type == SceneDebugUI::SelectionType::GameObject) ? sel.ptr : nullptr;
    GraphicsContext::GetInstance().GetCommandQueue()->SetWireframeSelectedObject(selPtr);

    ZNScene::Render();
}

void CCTVScene::RenderForward()
{
    ZNScene::RenderForward();

    // Cameras + Room Model in Outliner
    SceneDebugUI::Get().onOutlinerExtras = [this]() {
        auto& sel = SceneDebugUI::Get().GetSelection();

        if (ImGui::TreeNodeEx("Cameras", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool isSel = (sel.type == SceneDebugUI::SelectionType::Custom && sel.ptr == cctvCamera);
            if (ImGui::Selectable("CCTV Overhead", isSel))
            {
                sel.type = SceneDebugUI::SelectionType::Custom;
                sel.ptr  = cctvCamera;
            }
            ImGui::TreePop();
        }

        if (!room.objects.empty())
        {
            std::string label = "Room Model (" + std::to_string(room.objects.size()) + ")";
            if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_None))
            {
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
        }
    };

    // Inspector for CCTV overhead camera
    SceneDebugUI::Get().onInspectorExtras = [this](void* ptr) {
        ZNCamera* cam = static_cast<ZNCamera*>(ptr);
        ImGui::Text("Camera: CCTV Overhead");
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

    // Scene-specific Debug panel extras: room mesh count info
    if (!room.objects.empty())
    {
        SceneDebugUI::Get().onDebugExtras = [this]() {
            ImGui::Text("Room: %d meshes", (int)room.objects.size());
        };
    }
    else
    {
        SceneDebugUI::Get().onDebugExtras = nullptr;
    }
}
