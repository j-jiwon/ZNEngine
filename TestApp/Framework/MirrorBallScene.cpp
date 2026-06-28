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

    ZNCamera* cam = new ZNCamera();
    cam->SetPosition(ZNVector3(0.f, 2.f, -5.f));
    cam->SetRotation(-10.f, 0.f);
    cam->SetMoveSpeed(3.f);
    SetCamera(cam);

    ZNDirectionalLight* dirLight = Platform::CreateDirectionalLight();
    dirLight->SetDirection(ZNVector3(0.3f, -1.f, 0.5f));
    dirLight->SetIntensity(5.f);
    dirLight->SetColor(ZNVector3(0.9f, 0.9f, 1.f));
    dirLight->SetAmbientIntensity(0.5f);
    dirLight->SetShadowFocusPoint(ZNVector3(0.f, 1.f, 0.f));
    dirLight->SetShadowBounds(10.f, 0.1f, 30.f);
    SetDirectionalLight(dirLight);

    // 4 corner spotlights aimed at the ball center (0, 1, 0)
    static const struct { ZNVector3 pos; ZNVector3 color; } kLights[4] = {
        { ZNVector3(-3.f, 4.f, -3.f), ZNVector3(1.0f, 0.85f, 0.7f)  },
        { ZNVector3( 3.f, 4.f, -3.f), ZNVector3(0.7f, 0.85f, 1.0f)  },
        { ZNVector3(-3.f, 4.f,  3.f), ZNVector3(1.0f, 0.75f, 0.55f) },
        { ZNVector3( 3.f, 4.f,  3.f), ZNVector3(0.75f, 0.75f, 1.0f) },
    };
    static const ZNVector3 kBallCenter(0.f, 1.f, 0.f);
    for (int i = 0; i < 4; ++i)
    {
        ZNVector3 dir = (kBallCenter - kLights[i].pos).Normalize();
        spotLights[i] = Platform::CreateSpotLight();
        spotLights[i]->SetPosition(kLights[i].pos);
        spotLights[i]->SetDirection(dir);
        spotLights[i]->SetColor(kLights[i].color);
        spotLights[i]->SetIntensity(4.f);
        spotLights[i]->SetAmbientIntensity(0.f);
        spotLights[i]->SetCutoffAngle(18.f, 28.f);
        spotLights[i]->SetAttenuation(1.f, 0.045f, 0.0075f);
        AddSpotLight(spotLights[i]);
    }

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
        ZNMaterial* mat = ZNMaterialFactory::CreatePBR(defaultShader,
            ZNVector4(0.95f, 0.95f, 0.95f, 1.f), 0.95f, 0.1f);
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
            obj->GetTransform().position = ZNVector3(-1.f, 1.f, 0.f);
            obj->GetTransform().scale    = ZNVector3(0.01f, 0.01f, 0.01f);
            AddGameObject(obj);
            mirrorBall.objects.push_back(obj);
        }
    }

    // --- Glass ball: semi-transparent, forward pass with alpha blend ---
    {
        ZNMaterial* mat = ZNMaterialFactory::CreatePBR(glassShader,
            ZNVector4(0.8f, 0.9f, 1.f, 0.35f), 0.f, 0.15f);
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
            obj->GetTransform().position = ZNVector3(1.f, 1.f, 0.f);
            obj->GetTransform().scale    = ZNVector3(0.01f, 0.01f, 0.01f);
            obj->SetCastShadow(false);
            AddForwardGameObject(obj);
            glassBall.objects.push_back(obj);
        }
    }
}

void MirrorBallScene::Update(float deltaTime)
{
    ZNScene::Update(deltaTime);
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
