#include "MirrorBallScene.h"
#include "SceneDebugUI.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>

using namespace ZNFramework;

void MirrorBallScene::Initialize()
{
    defaultShader = Platform::CreateShader();
    defaultShader->Load(GetResourcePath() / L"Shaders" / L"deferred_lighting.hlsli");

    glassShader = Platform::CreateShader();
    glassShader->Load(GetResourcePath() / L"Shaders" / L"forward_lit.hlsli");
    glassShader->EnableAlphaBlend();
    glassShader->DisableDepthWrite();

    // room.glb real bounds: X:[-1.43,1.43] Y:[0.10(floor),1.87(ceiling)] Z:[-1.55,1.75]
    // room center: (0, -, 0.1). Everything below is placed relative to that box.
    ZNCamera* cam = new ZNCamera();
    cam->SetPosition(ZNVector3(3.844f, 3.603f, 4.086f));
    cam->SetRotation(-25.11f, -135.18f);
    cam->SetMoveSpeed(1.5f);
    SetCamera(cam);

    

    ZNDirectionalLight* dirLight = Platform::CreateDirectionalLight();
    dirLight->SetDirection(ZNVector3(0.3f, -1.f, 0.5f));
    dirLight->SetIntensity(0.f);
    dirLight->SetColor(ZNVector3(0.9f, 0.9f, 1.f));
    dirLight->SetAmbientIntensity(0.5f);
    dirLight->SetShadowFocusPoint(ZNVector3(0.f, 1.f, 0.f));
    dirLight->SetShadowBounds(10.f, 0.1f, 30.f);
    SetDirectionalLight(dirLight);

    // 4 corner spotlights near the ceiling, aimed at the mirror ball
    static const struct { ZNVector3 pos; ZNVector3 color; } kLights[4] = {
        { ZNVector3(-1.2f, 1.8f, -1.3f), ZNVector3(1.0f, 0.f, 0.f)  },
        { ZNVector3( 1.2f, 1.8f, -1.3f), ZNVector3(0.f, 1.0f, 0.f)  },
        { ZNVector3(-1.2f, 1.8f,  1.5f), ZNVector3(0.f, 0.f, 1.0f) },
        { ZNVector3( 1.2f, 1.8f,  1.5f), ZNVector3(1.0f, 0.0f, 1.0f) },
    };
    static const ZNVector3 kBallCenter(0.f, 1.55f, 0.1f);
    for (int i = 0; i < 4; ++i)
    {
        ZNVector3 dir = (kBallCenter - kLights[i].pos).Normalize();
        spotLights[i] = Platform::CreateSpotLight();
        spotLights[i]->SetPosition(kLights[i].pos);
        spotLights[i]->SetDirection(dir);
        spotLights[i]->SetColor(kLights[i].color);
        spotLights[i]->SetIntensity(4.f);
        spotLights[i]->SetAmbientIntensity(0.5f);
        spotLights[i]->SetCutoffAngle(5.f, 10.f);
        spotLights[i]->SetAttenuation(1.f, 0.045f, 0.0075f);
        AddSpotLight(spotLights[i]);
    }

    // Point light inside the glass ball — warm white "bulb" glow
    innerLight = Platform::CreatePointLight();
    innerLight->SetPosition(ZNVector3(-0.7f, 0.25f, -0.9f));  // glass ball center
    innerLight->SetColor(ZNVector3(1.f, 0.7f, 0.1f));  // warm white
    innerLight->SetIntensity(20.f);
    innerLight->SetRadius(6.f);
    innerLight->SetAttenuation(1.f, 0.22f, 0.20f);
    AddPointLight(innerLight);

    // Load FBX once; instantiate as two separate sets of GameObjects
    std::filesystem::path fbxPath =
        GetResourcePath() / L"Models" / L"MirrorBall" / L"mirrorball_a.fbx";

    std::vector<MeshData> meshes;
    if (std::filesystem::exists(fbxPath))
    {
        std::cout << "[MirrorBallScene] Loading mirrorball_a.fbx...\n";
        ZNModelLoader* loader = Platform::CreateModelLoader();
        ModelData modelData;
        if (loader->Load(fbxPath, modelData))
        {
            meshes = std::move(modelData.meshes);
            std::cout << "[MirrorBallScene] Loaded " << meshes.size() << " meshes.\n";
        }
        else
            std::cout << "[MirrorBallScene] Load failed.\n";
        delete loader;
    }
    else
        std::cout << "[MirrorBallScene] mirrorball_a.fbx not found at: " << fbxPath << '\n';

    // --- Mirror ball: Metallic=1.0, Roughness=0.0, deferred ---
    {
        MaterialData mirrorData;
        mirrorData.params.albedoColor = ZNVector4(0.95f, 0.95f, 0.95f, 1.f);
        mirrorData.params.metallic    = 0.95f;
        mirrorData.params.roughness   = 0.35f;
        ZNMaterial* mat = ZNMaterialFactory::CreatePBRFromData(defaultShader, mirrorData);
        mirrorBall.materials.push_back(mat);

        for (size_t i = 0; i < meshes.size(); ++i)
        {
            ZNMesh* mesh = Platform::CreateMesh();
            mesh->Init(meshes[i].vertices, meshes[i].indices);
            mesh->SetMaterial(mat);

            ZNGameObject* obj = new ZNGameObject();
            obj->SetMesh(mesh);
            obj->SetMaterial(mat);
            obj->SetName("MirrorBall_" + std::to_string(i));
            obj->GetTransform().position = ZNVector3(0.f, 1.55f, 0.1f); // hangs near ceiling center
            // Raw mesh radius is ~100.5 units (FBX authored in cm); 0.0018 -> ~18cm radius,
            // a normal disco-ball size for this room (ceiling at Y~1.87).
            obj->GetTransform().scale    = ZNVector3(0.0018f, 0.0018f, 0.0018f);
            AddGameObject(obj);
            mirrorBall.objects.push_back(obj);
        }
    }

    // --- Monster model (glTF binary) ---
    {
        std::filesystem::path monsterPath =
            GetResourcePath() / L"Models" / L"Monster_S_0.glb";

        if (std::filesystem::exists(monsterPath))
        {
            std::cout << "[MirrorBallScene] Loading Monster_S_0.glb...\n";
            ZNModelLoader* loader = Platform::CreateModelLoader();
            ModelData modelData;
            if (loader->Load(monsterPath, modelData))
            {
                // One material per glTF material slot; textures (embedded or file-based) are
                // loaded and bound automatically. Flat-color fallback only kicks in when a
                // slot has no albedo texture and its baked color is near-black.
                for (const auto& matData : modelData.materials)
                {
                    MaterialData patched = matData;
                    bool hasAlbedoTex =
                        !patched.texturePaths[static_cast<size_t>(TextureType::Albedo)].empty() ||
                        !patched.embeddedTextureData[static_cast<size_t>(TextureType::Albedo)].empty();

                    if (!hasAlbedoTex)
                    {
                        ZNVector4& albedo = patched.params.albedoColor;
                        float lum = albedo.x * 0.299f + albedo.y * 0.587f + albedo.z * 0.114f;
                        if (lum < 0.05f)
                            albedo = ZNVector4(0.8f, 0.75f, 0.70f, 1.0f); // fallback warm-white
                    }

                    monster.materials.push_back(ZNMaterialFactory::CreatePBRFromData(defaultShader, patched));
                }
                if (monster.materials.empty())
                {
                    MaterialData fallback;
                    fallback.params.albedoColor = ZNVector4(0.8f, 0.75f, 0.70f, 1.0f);
                    fallback.params.roughness   = 0.6f;
                    monster.materials.push_back(ZNMaterialFactory::CreatePBRFromData(defaultShader, fallback));
                }

                for (const auto& meshData : modelData.meshes)
                {
                    size_t matIdx = (meshData.materialIndex < monster.materials.size())
                                  ? meshData.materialIndex : 0;
                    ZNMaterial* mat = monster.materials[matIdx];

                    ZNMesh* mesh = Platform::CreateMesh();
                    mesh->Init(meshData.vertices, meshData.indices);
                    mesh->SetMaterial(mat);

                    ZNGameObject* obj = new ZNGameObject();
                    obj->SetMesh(mesh);
                    obj->SetMaterial(mat);
                    obj->SetName("Monster_" + std::to_string(monster.objects.size()));
                    obj->GetTransform().position = ZNVector3(0.7f, 1.1f, 1.0f); // standing on the real floor (Y~0.1)
                    AddGameObject(obj);
                    monster.objects.push_back(obj);
                }
                std::cout << "[MirrorBallScene] Monster loaded: " << monster.objects.size()
                          << " meshes, " << monster.materials.size() << " materials.\n";
            }
            else
                std::cout << "[MirrorBallScene] Failed to load Monster_S_0.glb.\n";
            delete loader;
        }
        else
            std::cout << "[MirrorBallScene] Monster_S_0.glb not found at: " << monsterPath << '\n';
    }

    // --- Glass ball: semi-transparent, forward pass with alpha blend ---
    {
        MaterialData glassData;
        glassData.params.albedoColor = ZNVector4(0.8f, 0.9f, 1.f, 0.35f);
        glassData.params.metallic    = 0.f;
        glassData.params.roughness   = 0.15f;
        ZNMaterial* mat = ZNMaterialFactory::CreatePBRFromData(glassShader, glassData);
        glassBall.materials.push_back(mat);

        for (size_t i = 0; i < meshes.size(); ++i)
        {
            ZNMesh* mesh = Platform::CreateMesh();
            mesh->Init(meshes[i].vertices, meshes[i].indices);
            mesh->SetMaterial(mat);

            ZNGameObject* obj = new ZNGameObject();
            obj->SetMesh(mesh);
            obj->SetMaterial(mat);
            obj->SetName("GlassBall_" + std::to_string(i));
            obj->GetTransform().position = ZNVector3(-0.7f, 0.25f, -0.9f); // open floor area near the door
            obj->GetTransform().scale    = ZNVector3(0.0015f, 0.0015f, 0.0015f); // ~15cm radius decorative sphere
            obj->SetCastShadow(false);
            AddForwardGameObject(obj);
            glassBall.objects.push_back(obj);
        }
    }

    // --- Room model (glTF binary background) ---
    {
        std::filesystem::path roomPath =
            GetResourcePath() / L"Models" / L"room.glb";

        if (std::filesystem::exists(roomPath))
        {
            std::cout << "[MirrorBallScene] Loading room.glb...\n";
            ZNModelLoader* loader = Platform::CreateModelLoader();
            ModelData modelData;
            if (loader->Load(roomPath, modelData))
            {
                // One material per glTF material slot; textures (embedded or file-based) are
                // loaded and bound automatically. Flat-color fallback only kicks in when a
                // slot has no albedo texture and its baked color is near-black.
                for (const auto& matData : modelData.materials)
                {
                    MaterialData patched = matData;
                    bool hasAlbedoTex =
                        !patched.texturePaths[static_cast<size_t>(TextureType::Albedo)].empty() ||
                        !patched.embeddedTextureData[static_cast<size_t>(TextureType::Albedo)].empty();

                    if (!hasAlbedoTex)
                    {
                        ZNVector4& albedo = patched.params.albedoColor;
                        float lum = albedo.x * 0.299f + albedo.y * 0.587f + albedo.z * 0.114f;
                        if (lum < 0.05f)
                            albedo = ZNVector4(0.8f, 0.75f, 0.70f, 1.0f); // fallback warm-white
                        if (patched.params.roughness < 0.25f)
                            patched.params.roughness = 0.25f;
                    }

                    room.materials.push_back(ZNMaterialFactory::CreatePBRFromData(defaultShader, patched));
                }
                if (room.materials.empty())
                {
                    MaterialData fallback;
                    fallback.params.albedoColor = ZNVector4(0.8f, 0.75f, 0.70f, 1.0f);
                    fallback.params.roughness   = 0.6f;
                    room.materials.push_back(ZNMaterialFactory::CreatePBRFromData(defaultShader, fallback));
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
                    AddGameObject(obj);
                    room.objects.push_back(obj);
                }
                std::cout << "[MirrorBallScene] Room loaded: " << room.objects.size()
                          << " meshes, " << room.materials.size() << " materials.\n";
            }
            else
                std::cout << "[MirrorBallScene] Failed to load room.glb.\n";
            delete loader;
        }
        else
            std::cout << "[MirrorBallScene] room.glb not found at: " << roomPath << '\n';
    }
}

void MirrorBallScene::Update(float deltaTime)
{
    ZNScene::Update(deltaTime);

    for (auto* obj : mirrorBall.objects)
        obj->GetTransform().rotation.y += 30.f * deltaTime;

    for (auto* obj : glassBall.objects)
        obj->GetTransform().rotation.y -= 20.f * deltaTime;
}

void MirrorBallScene::Render()
{
    auto& sel = SceneDebugUI::Get().GetSelection();
    void* selPtr = (sel.type == SceneDebugUI::SelectionType::GameObject) ? sel.ptr : nullptr;
    GraphicsContext::GetInstance().GetCommandQueue()->SetWireframeSelectedObject(selPtr);

    ZNScene::Render();
}

void MirrorBallScene::RenderForward()
{
    ZNScene::RenderForward();

    // No scene-specific debug extras — spotlight indicators handled by SceneDebugUI
    SceneDebugUI::Get().onDebugExtras = nullptr;
}
