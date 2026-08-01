#include "VehicleScene.h"
#include "SceneDebugUI.h"
#include <imgui.h>
#include <string>
#include <cmath>

using namespace ZNFramework;
using Vehicle::ObjectClass;

// ---- construction / teardown -----------------------------------------------------------------

VehicleScene::~VehicleScene()
{
    // ~ZNScene frees the game objects; the meshes/materials are ours (~ZNGameObject won't touch them).
    for (auto* mat  : ownedMaterials) delete mat;
    for (auto* mesh : ownedMeshes)    delete mesh;
}

ZNMesh* VehicleScene::GetClassMesh(ObjectClass c) const
{
    return classRes[static_cast<int>(c)].mesh;
}

ZNMaterial* VehicleScene::GetClassMaterial(ObjectClass c) const
{
    return classRes[static_cast<int>(c)].mat;
}

// ---- setup -----------------------------------------------------------------------------------

void VehicleScene::Initialize()
{
    mainShader = Platform::CreateShader();
    mainShader->Load(GetResourcePath() / L"Shaders" / L"deferred_lighting.hlsli");

    // Chase camera: behind + above the ego, looking down +Z. (Projection is overwritten each frame
    // by ApplicationContext — far plane 100 covers the road.)
    ZNCamera* cam = new ZNCamera();
    cam->SetPosition(ZNVector3(Vehicle::SyntheticSource::kEgoLaneX, 5.5f, -12.0f)); // behind the ego lane
    cam->SetRotation(-15.0f, 0.0f);     // pitch down 15deg, looking straight ahead
    cam->SetMoveSpeed(8.0f);
    SetCamera(cam);

    // Sun.
    ZNDirectionalLight* dirLight = Platform::CreateDirectionalLight();
    dirLight->SetDirection(ZNVector3(0.35f, -1.0f, 0.45f));
    dirLight->SetIntensity(2.6f);
    dirLight->SetColor(ZNVector3(1.0f, 0.98f, 0.92f));
    dirLight->SetAmbientIntensity(0.7f);
    dirLight->SetShadowFocusPoint(ZNVector3(0.0f, 0.0f, 20.0f));
    dirLight->SetShadowBounds(70.0f, 0.1f, 160.0f);
    SetDirectionalLight(dirLight);

    BuildClassResources();
    BuildStaticStage();

    dataSource = std::make_unique<Vehicle::SyntheticSource>(/*agentCount*/ 14);
    interpolator.Init(*dataSource, dataSource->GetSensorHz());
}

void VehicleScene::BuildClassResources()
{
    // One unit cube (edge 1: CreateCube(0.5) spans -0.5..0.5) per class, its class material bound.
    struct Def { ObjectClass cls; ZNVector4 color; float metallic; float roughness; } defs[] = {
        { ObjectClass::Car,        ZNVector4(0.10f, 0.68f, 0.60f, 1.0f), 0.1f, 0.45f }, // teal
        { ObjectClass::Pedestrian, ZNVector4(0.95f, 0.45f, 0.35f, 1.0f), 0.0f, 0.75f }, // coral
        { ObjectClass::Cyclist,    ZNVector4(0.95f, 0.62f, 0.15f, 1.0f), 0.0f, 0.60f }, // amber
    };

    for (const auto& d : defs)
    {
        ZNMaterial* mat  = ZNMaterialFactory::CreatePBR(mainShader, d.color, d.metallic, d.roughness);
        ZNMesh*     mesh = ZNMeshFactory::CreateCube(0.5f);
        mesh->SetMaterial(mat);

        ClassRes& r = classRes[static_cast<int>(d.cls)];
        r.mat  = mat;
        r.mesh = mesh;
        ownedMaterials.push_back(mat);
        ownedMeshes.push_back(mesh);
    }
}

void VehicleScene::BuildStaticStage()
{
    // --- Ground (asphalt) ---
    ZNMaterial* groundMat = ZNMaterialFactory::CreatePBR(
        mainShader, ZNVector4(0.07f, 0.07f, 0.08f, 1.0f), 0.0f, 0.95f);
    ZNMesh* groundMesh = ZNMeshFactory::CreatePlane(120.0f); // spans +-120 in X and Z
    groundMesh->SetMaterial(groundMat);
    ownedMaterials.push_back(groundMat);
    ownedMeshes.push_back(groundMesh);

    ZNGameObject* ground = new ZNGameObject();
    ground->SetMesh(groundMesh);
    ground->SetMaterial(groundMat);
    ground->SetName("Ground");
    ground->SetTag("Ground");
    ground->SetCastShadow(false);
    AddGameObject(ground);

    // --- Ego (blue box, fixed at origin) ---
    ZNMaterial* egoMat = ZNMaterialFactory::CreatePBR(
        mainShader, ZNVector4(0.16f, 0.38f, 0.92f, 1.0f), 0.15f, 0.40f); // blue
    ZNMesh* egoMesh = ZNMeshFactory::CreateCube(0.5f);
    egoMesh->SetMaterial(egoMat);
    ownedMaterials.push_back(egoMat);
    ownedMeshes.push_back(egoMesh);

    ego = new ZNGameObject();
    ego->SetMesh(egoMesh);
    ego->SetMaterial(egoMat);
    ego->SetName("EGO");
    ego->SetTag("Ego");
    ego->GetTransform().scale    = ZNVector3(1.9f, 1.5f, Vehicle::SyntheticSource::kEgoLen);
    ego->GetTransform().position = ZNVector3(Vehicle::SyntheticSource::kEgoLaneX, 0.75f, 0.0f); // right lane
    AddGameObject(ego);

    // --- Scrolling lane dashes (shared mesh + bright material) ---
    ZNMaterial* laneMat = ZNMaterialFactory::CreatePBR(
        mainShader, ZNVector4(0.95f, 0.85f, 0.25f, 1.0f), 0.0f, 0.5f); // road-marking yellow
    ZNMesh* laneMesh = ZNMeshFactory::CreateCube(0.5f);
    laneMesh->SetMaterial(laneMat);
    ownedMaterials.push_back(laneMat);
    ownedMeshes.push_back(laneMesh);

    const float spacing = 6.0f;
    const int   perLine = 18;
    const float lineX[] = { 0.0f, 3.75f, -3.75f };
    laneCycle = spacing * perLine;

    for (float x : lineX)
    {
        for (int i = 0; i < perLine; ++i)
        {
            ZNGameObject* dash = new ZNGameObject();
            dash->SetMesh(laneMesh);
            dash->SetMaterial(laneMat);
            dash->SetName("Lane");
            dash->SetTag("Lane");
            dash->SetCastShadow(false);
            dash->GetTransform().scale    = ZNVector3(0.18f, 0.02f, 3.0f);
            dash->GetTransform().position = ZNVector3(x, 0.02f, roadStartZ + i * spacing);
            AddGameObject(dash);

            laneDashes.push_back(dash);
            laneDashBaseZ.push_back(static_cast<float>(i) * spacing);
        }
    }
}

// ---- per-frame -------------------------------------------------------------------------------

void VehicleScene::Update(float deltaTime)
{
    ZNScene::Update(deltaTime);

    if (dataSource)
    {
        // Interpolator ticks the source at fixed sensor cadence, then resamples to this render frame.
        interpolator.Update(deltaTime);
        binding.Apply(interpolator.Sample(), *this);

        // Lane dashes flow toward the ego at the ego's forward speed (unless paused).
        const float egoSpeed = dataSource->GetEgoSpeed();
        const float scroll = dataSource->IsPaused()
            ? 0.0f
            : egoSpeed * deltaTime * dataSource->GetSpeed();
        UpdateLaneDashes(scroll);
    }
}

void VehicleScene::UpdateLaneDashes(float scrollDelta)
{
    if (laneCycle <= 0.0f) return;
    laneScroll = std::fmod(laneScroll + scrollDelta, laneCycle);

    for (size_t i = 0; i < laneDashes.size(); ++i)
    {
        float z = std::fmod(laneDashBaseZ[i] - laneScroll, laneCycle);
        if (z < 0.0f) z += laneCycle;
        laneDashes[i]->GetTransform().position.z = roadStartZ + z;
    }
}

void VehicleScene::Render()
{
    // Highlight the Outliner-selected object with a wireframe overlay (same as CCTVScene).
    auto& sel = SceneDebugUI::Get().GetSelection();
    void* selPtr = (sel.type == SceneDebugUI::SelectionType::GameObject) ? sel.ptr : nullptr;
    GraphicsContext::GetInstance().GetCommandQueue()->SetWireframeSelectedObject(selPtr);

    ZNScene::Render();
}

void VehicleScene::RenderForward()
{
    ZNScene::RenderForward();

    // Outliner: list the live tracks (spawned/destroyed by SceneBinding each frame).
    SceneDebugUI::Get().onOutlinerExtras = [this]() {
        auto& sel = SceneDebugUI::Get().GetSelection();

        int trackCount = 0;
        for (auto* obj : GetGameObjects())
            if (obj && obj->GetTag() == "Track") ++trackCount;

        std::string label = "Tracked Objects (" + std::to_string(trackCount) + ")";
        if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            // "Ego" first, then tracks.
            if (ego)
            {
                bool isSel = (sel.type == SceneDebugUI::SelectionType::GameObject && sel.ptr == ego);
                if (ImGui::Selectable("EGO", isSel))
                {
                    sel.type = SceneDebugUI::SelectionType::GameObject;
                    sel.ptr  = ego;
                }
            }
            for (auto* obj : GetGameObjects())
            {
                if (!obj || obj->GetTag() != "Track") continue;
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

    if (!SceneDebugUI::Get().IsVisible()) return;
    RenderDataSourcePanel();
}

void VehicleScene::RenderDataSourcePanel()
{
    if (!dataSource) return;

    const Vehicle::FrameData& frame = dataSource->GetCurrentFrame();

    ImGui::Begin("DataSource");

    ImGui::Text("Source: %s", dataSource->GetName());
    ImGui::SameLine();
    bool paused = dataSource->IsPaused();
    if (ImGui::Button(paused ? "Play" : "Pause"))
        dataSource->SetPaused(!paused);

    float spd = dataSource->GetSpeed();
    if (ImGui::SliderFloat("Speed", &spd, 0.25f, 4.0f, "%.2fx"))
        dataSource->SetSpeed(spd);

    ImGui::Text("t = %.1f s", frame.timestamp);

    ImGui::Separator();
    ImGui::Text("Ego speed : %.1f m/s (%.0f km/h)",
                dataSource->GetEgoSpeed(), dataSource->GetEgoSpeed() * 3.6f);
    ImGui::Text("Tracked   : %d", binding.LiveTrackCount());
    ImGui::Text("Draw calls: %d", ZNGameObject::GetLastFrameDrawCalls());

    // --- Temporal resample: discrete sensor -> render fps (stage 3) ---
    ImGui::Separator();
    ImGui::TextUnformatted("Temporal resample");

    using Mode = Vehicle::FrameInterpolator::Mode;
    int m = static_cast<int>(interpolator.GetMode());
    const char* modes[] = { "Off (raw sensor rate)", "Interpolate", "Extrapolate" };
    if (ImGui::Combo("Mode", &m, modes, IM_ARRAYSIZE(modes)))
        interpolator.SetMode(static_cast<Mode>(m));

    if (interpolator.GetMode() == Mode::Interpolate)
    {
        float d = interpolator.GetInterpDelayPeriods();
        if (ImGui::SliderFloat("Interp delay", &d, 0.5f, 2.0f, "%.2f periods"))
            interpolator.SetInterpDelayPeriods(d);
    }

    // Sensor cadence + imperfections (lower Hz / raise dropout/jitter to make the modes diverge).
    float hz = interpolator.GetSensorHz();
    if (ImGui::SliderFloat("Sensor Hz", &hz, 4.0f, 60.0f, "%.0f Hz"))
        interpolator.SetSensorHz(hz);
    float dropPct = interpolator.GetDropoutProb() * 100.0f;
    if (ImGui::SliderFloat("Dropout", &dropPct, 0.0f, 60.0f, "%.0f%%"))
        interpolator.SetDropoutProb(dropPct * 0.01f);
    float jit = interpolator.GetJitterFrac();
    if (ImGui::SliderFloat("Jitter", &jit, 0.0f, 0.9f, "%.2f period"))
        interpolator.SetJitterFrac(jit);

    ImGui::Text("Sensor    : %.0f Hz (%.1f ms/tick)",
                interpolator.GetSensorHz(), 1000.0f / interpolator.GetSensorHz());
    ImGui::Text("Buffered  : %d snapshots", interpolator.BufferedFrames());
    ImGui::Text("Added lat.: %.1f ms", interpolator.EffectiveLatencyMs());
    ImGui::Text("Stale     : %.0f ms (age of newest sample)", interpolator.StaleMs());

    ImGui::End();
}
