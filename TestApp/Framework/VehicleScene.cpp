#include "VehicleScene.h"
#include "SceneDebugUI.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/RenderTexture.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/CommandQueue.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/GraphicsDevice.h"
#include "ZNFramework/UI/Platform/Win32_DX12/ImGuiLayer.h"
#include "ZNFramework/UI/ImGuiAnchor.h"
#include "ZNFramework/ZNLog.h"
#include <imgui.h>
#include <string>
#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace ZNFramework;
using Vehicle::ObjectClass;


// ---- construction / teardown -----------------------------------------------------------------

VehicleScene::~VehicleScene()
{
    // ~ZNScene frees the game objects; the meshes/materials are ours (~ZNGameObject won't touch them).
    for (auto* mat  : ownedMaterials) delete mat;
    for (auto* mesh : ownedMeshes)    delete mesh;
    for (auto& sv   : surroundViews) { delete sv.rt; delete sv.cam; }
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

    // Forward shader for the offscreen surround/top-down RTs (same approach as CCTVScene).
    offscreenShader = Platform::CreateShader();
    offscreenShader->Load(GetResourcePath() / L"Shaders" / L"forward_pbr.hlsli");

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
    dirLight->SetIntensity(0.7f);
    dirLight->SetColor(ZNVector3(1.0f, 0.98f, 0.92f));
    dirLight->SetAmbientIntensity(0.7f);
    dirLight->SetShadowFocusPoint(ZNVector3(0.0f, 0.0f, 20.0f));
    dirLight->SetShadowBounds(70.0f, 0.1f, 160.0f);
    SetDirectionalLight(dirLight);

    // Studio look (Tesla-nav style): wrap the scene in a flat neutral grey instead of a photographic
    // sky. A uniform grey panorama bakes to uniform IBL -> flat ambient fill (no more near-black
    // ground), and the same image as the skybox gives an even grey background.
    SetEnvCubemapTexture(GetResourcePath() / L"Textures" / L"studio_grey.jpg");
    SetSkyboxTexture(GetResourcePath() / L"Textures" / L"studio_grey.jpg");

    BuildClassResources();
    BuildStaticStage();
    BuildSurroundViews();

    synthetic = std::make_unique<Vehicle::SyntheticSource>();
    UseSource(synthetic.get());
}

// Swap the active data source (Live synthetic <-> Log playback) and re-prime the interpolator with
// its sensor rate. The rest of the pipeline (binding, scene, panel) is source-agnostic.
void VehicleScene::UseSource(Vehicle::IDataSource* src)
{
    if (!src) return;
    dataSource = src;
    interpolator.Init(*dataSource, dataSource->GetSensorHz());
    scrubbing = false;
}

// ---- live recording (capture the synthetic sensor stream forward from "now") ------------------

void VehicleScene::StartRecording()
{
    recording     = true;
    recordFirstTs = 0.0f;
    recordLastTs  = -1.0f;
    recordBuffer.clear();
    recordStatus.clear();
}

void VehicleScene::TickRecording()
{
    // Only the live synthetic stream is recordable; grab each NEW sensor frame (dedup by timestamp,
    // since Update() runs far faster than the sensor rate). Paused -> no new frames -> capture waits.
    if (!recording || dataSource != synthetic.get()) return;

    const Vehicle::FrameData& f = synthetic->GetCurrentFrame();
    if (f.timestamp <= recordLastTs + 1e-4f) return;

    if (recordBuffer.empty()) recordFirstTs = f.timestamp;
    Vehicle::FrameData copy = f;
    copy.timestamp = f.timestamp - recordFirstTs;   // re-base so the log starts at t=0
    const float elapsed = copy.timestamp;
    recordBuffer.push_back(std::move(copy));
    recordLastTs = f.timestamp;

    if (elapsed >= recordTarget) FinishRecording();
}

void VehicleScene::FinishRecording()
{
    recording = false;
    if (recordBuffer.empty()) { recordStatus = "no frames captured"; return; }

    Vehicle::LogData log;
    log.sensorHz = interpolator.GetSensorHz();
    log.frames   = std::move(recordBuffer);
    recordBuffer.clear();

    const auto path = GetResourcePath() / L"Logs" / L"scenario.json";
    recordStatus = Vehicle::WriteLog(path, log)
        ? "saved " + std::to_string(log.frames.size()) + " frames -> scenario.json"
        : "save FAILED";
}

void VehicleScene::BuildClassResources()
{
    // One unit cube (edge 1: CreateCube(0.5) spans -0.5..0.5) per class, its class material bound.
    // Car's is the plain-cube fallback (no car model loaded); Pedestrian/Cyclist's is the body box,
    // scaled/positioned per-instance in SpawnHumanoidInstance to read as a person, not a flat box.
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

        if (d.cls == ObjectClass::Pedestrian || d.cls == ObjectClass::Cyclist)
        {
            ZNMesh* head = ZNMeshFactory::CreateSphere(0.5f);
            head->SetMaterial(mat);
            r.headMesh = head;
            ownedMeshes.push_back(head);
        }
    }
}

// Spawns a pedestrian/cyclist as root + body + head (+ a bike-frame bar for cyclist, reusing the
// body mesh/material) — same "shared mesh, N instances" pattern as SpawnCarInstance, so GBuffer
// instancing batches every pedestrian/cyclist part the same way it batches car parts.
ZNObjectHandle VehicleScene::SpawnHumanoidInstance(ObjectClass cls, const std::string& name, const std::string& tag)
{
    const ClassRes& r = classRes[static_cast<int>(cls)];
    const bool isCyclist = (cls == ObjectClass::Cyclist);

    ZNGameObject* root = new ZNGameObject();
    root->SetName(name);
    root->SetTag(tag);
    ZNObjectHandle rootHandle = AddGameObject(root);

    auto addPart = [&](ZNMesh* mesh, ZNVector3 pos, ZNVector3 rot, ZNVector3 scale, const char* partName)
    {
        ZNGameObject* part = new ZNGameObject();
        part->SetMesh(mesh);
        part->SetMaterial(r.mat);
        part->SetName(name + partName);
        part->SetTag("HumanoidPart");
        part->SetCastShadow(true);
        part->GetTransform().position = pos;
        part->GetTransform().rotation = rot;
        part->GetTransform().scale    = scale;
        root->AddChild(part);
        AddGameObject(part);
    };

    if (isCyclist)
    {
        // Leaning-forward rider: tilted torso + head carried forward with it, plus a low horizontal
        // bar standing in for the bike frame/wheels so the silhouette doesn't read as just a person.
        addPart(r.mesh,     ZNVector3(0.0f, 0.85f, 0.05f), ZNVector3(-20.0f, 0.0f, 0.0f), ZNVector3(0.35f, 1.0f, 0.3f),  "_body");
        addPart(r.headMesh, ZNVector3(0.0f, 1.48f, 0.28f), ZNVector3(0.0f, 0.0f, 0.0f),             ZNVector3(0.4f,  0.4f, 0.4f),  "_head");
        addPart(r.mesh,     ZNVector3(0.0f, 0.42f, 0.0f),  ZNVector3(0.0f, 0.0f, 0.0f),             ZNVector3(0.18f, 0.18f, 1.3f), "_frame");
    }
    else
    {
        addPart(r.mesh,     ZNVector3(0.0f, 0.65f, 0.0f), ZNVector3(0.0f, 0.0f, 0.0f), ZNVector3(0.4f,  1.3f, 0.35f), "_body");
        addPart(r.headMesh, ZNVector3(0.0f, 1.52f, 0.0f), ZNVector3(0.0f, 0.0f, 0.0f), ZNVector3(0.44f, 0.44f, 0.44f), "_head");
    }

    return rootHandle;
}

// Load a low-poly car glb into shared meshes/materials + a fit transform. Vertices are pre-baked
// into model space by the loader (aiProcess_PreTransformVertices), so children render at identity
// under a scaled root. targetLen is the desired real-world length (metres).
bool VehicleScene::LoadCarModel(const std::filesystem::path& path, float targetLen, CarModel& out, bool isEgo)
{
    if (!std::filesystem::exists(path))
    {
        ZNLOG_WARN(LogChannel::Scene, "car model not found: %s", path.string().c_str());
        return false;
    }

    ZNModelLoader* loader = Platform::CreateModelLoader();
    ModelData modelData;
    const bool ok = loader->Load(path, modelData);
    delete loader;
    if (!ok || modelData.meshes.empty()) return false;

    // Regular traffic intentionally uses a flat material; only the ego keeps the model textures.
    if (isEgo)
    {
        for (const auto& matData : modelData.materials)
        {
            ZNMaterial* m = ZNMaterialFactory::CreatePBRFromData(mainShader, matData);
            out.mats.push_back(m);
            ownedMaterials.push_back(m);
        }
    }
    if (out.mats.empty())
    {
        ZNMaterial* m = ZNMaterialFactory::CreatePBR(mainShader, ZNVector4(0.8f, 0.8f, 0.8f, 1.0f), 0.1f, 0.5f);
        out.mats.push_back(m);
        ownedMaterials.push_back(m);
    }

    // Meshes + model-space bounds (for a uniform fit scale + ground offset).
    float minX = 1e9f, minY = 1e9f, minZ = 1e9f, maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;
    for (const auto& md : modelData.meshes)
    {
        ZNMesh* mesh = Platform::CreateMesh();
        mesh->Init(md.vertices, md.indices);
        const size_t mi = (md.materialIndex < out.mats.size()) ? md.materialIndex : 0;
        mesh->SetMaterial(out.mats[mi]);
        out.meshes.push_back(mesh);
        out.meshMat.push_back(static_cast<int>(mi));
        ownedMeshes.push_back(mesh);

        for (const auto& v : md.vertices)
        {
            minX = (std::min)(minX, v.pos.x); maxX = (std::max)(maxX, v.pos.x);
            minY = (std::min)(minY, v.pos.y); maxY = (std::max)(maxY, v.pos.y);
            minZ = (std::min)(minZ, v.pos.z); maxZ = (std::max)(maxZ, v.pos.z);
        }
    }

    const float lenX = maxX - minX, lenZ = maxZ - minZ;
    const float horiz = (std::max)(lenX, lenZ);
    out.fitScale        = (horiz > 1e-4f) ? targetLen / horiz : 1.0f;
    out.groundLift      = -minY * out.fitScale;
    // Longest horizontal axis = the car's front-back axis; align it to world +Z. glTF's convention
    // puts an asset's local forward on -Z, so an extra 180 flips the nose to point down +Z (tune if
    // a future asset doesn't follow the convention).
    out.modelForwardYaw = (lenX > lenZ) ? -90.0f : 180.0f;
    out.valid           = true;

    ZNLOG_INFO(LogChannel::Scene, "car '%s': %zu meshes, fitScale %.3f, lenX %.2f lenZ %.2f, yaw %.0f",
               path.filename().string().c_str(), out.meshes.size(), out.fitScale, lenX, lenZ, out.modelForwardYaw);
    return true;
}

// Spawn a shared-mesh car instance: a scaled root + one child per model mesh (sharing meshes/mats).
// Caller sets root position/rotation. Children tagged "CarPart" so they don't count as tracks.
// Returns the ROOT's handle, not a raw pointer -- track instances need it to Resolve/Destroy safely
// later (AddModelRoot doesn't hand the handle back).
ZNObjectHandle VehicleScene::SpawnCarInstance(const CarModel& car, const std::string& name, const std::string& tag)
{
    ZNGameObject* root = new ZNGameObject();
    root->SetName(name);
    root->GetTransform().scale = ZNVector3(car.fitScale, car.fitScale, car.fitScale);
    root->SetTag(tag);
    ZNObjectHandle rootHandle = AddGameObject(root);

    for (size_t i = 0; i < car.meshes.size(); ++i)
    {
        ZNGameObject* part = new ZNGameObject();
        part->SetMesh(car.meshes[i]);
        part->SetMaterial(car.mats[car.meshMat[i]]);
        part->SetName(name + "_part" + std::to_string(i));
        part->SetTag("CarPart");
        part->SetCastShadow(true);
        root->AddChild(part);
        AddGameObject(part);
    }
    return rootHandle;
}

// Car-class track: shared model, default (white) materials. SceneBinding calls this instead of
// spawning a plain cube once a car model is loaded.
ZNObjectHandle VehicleScene::SpawnCarTrack(const std::string& name)
{
    return SpawnCarInstance(carModel, name, "Track");
}

void VehicleScene::ApplyEgoPaint()
{
    if (!egoCarModel.valid) return;

    static const ZNVector4 kEgoPaint(0.75f, 0.06f, 0.05f, 1.0f);
    for (size_t i = 0; i < egoCarModel.mats.size(); ++i)
    {
        MaterialParams p = egoBaseMatParams[i];
        if (i == 0 && p.useAlbedoTexture < 0.5f)
            p.albedoColor = kEgoPaint;
        egoCarModel.mats[i]->SetParams(p);
    }
}

void VehicleScene::BuildStaticStage()
{
    // --- Ground (asphalt) ---
    ZNMaterial* groundMat = ZNMaterialFactory::CreatePBR(
        mainShader, ZNVector4(0.34f, 0.34f, 0.36f, 1.0f), 0.0f, 0.9f);
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

    // --- Car model: plain copy for Car-class tracks, a second copy for the ego (its
    // body-shell material gets painted red -- see ApplyEgoPaint) ---
    LoadCarModel(GetResourcePath() / L"Models" / L"sedan-sports.glb", Vehicle::SyntheticSource::kEgoLen, carModel);
    LoadCarModel(GetResourcePath() / L"Models" / L"sedan-sports.glb", Vehicle::SyntheticSource::kEgoLen, egoCarModel, true);
    if (egoCarModel.valid)
    {
        for (ZNMaterial* m : egoCarModel.mats)
            egoBaseMatParams.push_back(m->GetParams());

        ego = Resolve(SpawnCarInstance(egoCarModel, "EGO", "Ego"));
        ego->GetTransform().position = ZNVector3(Vehicle::SyntheticSource::kEgoLaneX, egoCarModel.groundLift, 0.0f);
        ego->GetTransform().rotation = ZNVector3(0.0f, egoCarModel.modelForwardYaw, 0.0f); // face +Z (forward)
        ApplyEgoPaint();
    }
    else
    {
        // Fallback: red box if the model is missing.
        ZNMaterial* egoMat = ZNMaterialFactory::CreatePBR(
            mainShader, ZNVector4(0.75f, 0.06f, 0.05f, 1.0f), 0.15f, 0.40f);
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
        ego->GetTransform().position = ZNVector3(Vehicle::SyntheticSource::kEgoLaneX, 0.75f, 0.0f);
        AddGameObject(ego);
    }

    // --- Lane markings ---
    // 4 lanes: [(solid) 차선1 (dash) 차선2 ((double-yellow)) 차선3(ego) (dash) 차선4 (solid)].
    // Edges + the centre double-yellow are one long quad each (uniform along Z, so scrolling them is
    // invisible -> keep them static); only the two dashed dividers scroll, which conveys the ego's motion.
    using SS = Vehicle::SyntheticSource;
    const float halfW     = SS::kLaneWidth * 0.5f;
    const float leftEdge  = SS::kLaneX[0] - halfW;
    const float rightEdge = SS::kLaneX[3] + halfW;
    const float dashL     = (SS::kLaneX[0] + SS::kLaneX[1]) * 0.5f; // 차선1|2
    const float centreX   = (SS::kLaneX[1] + SS::kLaneX[2]) * 0.5f; // 차선2|3 (oncoming vs forward)
    const float dashR     = (SS::kLaneX[2] + SS::kLaneX[3]) * 0.5f; // 차선3|4

    ZNMaterial* whiteMat  = ZNMaterialFactory::CreatePBR(
        mainShader, ZNVector4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 0.5f);
    // Deep-saturated yellow (low blue) so the bright grey ambient can't lift it toward pale/white.
    ZNMaterial* yellowMat = ZNMaterialFactory::CreatePBR(
        mainShader, ZNVector4(0.95f, 0.68f, 0.04f, 1.0f), 0.0f, 0.5f);
    // Material is mesh-bound, so one cube mesh per colour; per-object transforms give the line shapes.
    ZNMesh* whiteMesh  = ZNMeshFactory::CreateCube(0.5f); whiteMesh->SetMaterial(whiteMat);
    ZNMesh* yellowMesh = ZNMeshFactory::CreateCube(0.5f); yellowMesh->SetMaterial(yellowMat);
    ownedMaterials.push_back(whiteMat);  ownedMaterials.push_back(yellowMat);
    ownedMeshes.push_back(whiteMesh);    ownedMeshes.push_back(yellowMesh);

    // Set BOTH the mesh material (used by the main deferred pass) and the GameObject material: the
    // offscreen/surround pass keys off GetMaterial() and skips objects without one (see ZNScene.cpp).
    const float roadLen = 120.0f, roadMidZ = 30.0f;
    auto addSolid = [&](float x, ZNMesh* mesh, ZNMaterial* mat, float width) {
        ZNGameObject* ln = new ZNGameObject();
        ln->SetMesh(mesh);
        ln->SetMaterial(mat);
        ln->SetName("LaneLine"); ln->SetTag("Lane"); ln->SetCastShadow(false);
        ln->GetTransform().scale    = ZNVector3(width, 0.02f, roadLen);
        ln->GetTransform().position = ZNVector3(x, 0.02f, roadMidZ);
        AddGameObject(ln);
    };
    addSolid(leftEdge,        whiteMesh,  whiteMat,  0.24f);
    addSolid(rightEdge,       whiteMesh,  whiteMat,  0.24f);
    addSolid(centreX - 0.22f, yellowMesh, yellowMat, 0.16f);   // double yellow, inner line
    addSolid(centreX + 0.22f, yellowMesh, yellowMat, 0.16f);   // double yellow, outer line

    // Scrolling dashed dividers between the two same-direction lane pairs.
    const float spacing = 6.0f;
    const int   perLine = 18;
    const float dashX[] = { dashL, dashR };
    laneCycle = spacing * perLine;

    for (float x : dashX)
    {
        for (int i = 0; i < perLine; ++i)
        {
            ZNGameObject* dash = new ZNGameObject();
            dash->SetMesh(whiteMesh);
            dash->SetMaterial(whiteMat);
            dash->SetName("Lane"); dash->SetTag("Lane"); dash->SetCastShadow(false);
            dash->GetTransform().scale    = ZNVector3(0.20f, 0.02f, 3.0f);
            dash->GetTransform().position = ZNVector3(x, 0.02f, roadStartZ + i * spacing);
            AddGameObject(dash);

            laneDashes.push_back(dash);
            laneDashBaseZ.push_back(static_cast<float>(i) * spacing);
        }
    }
}

// Multi-camera surround view (stage 4). The ego is fixed at the origin, so these cameras are
// static — no per-frame update. Each renders the whole scene into its own RT via the shared
// OffscreenCameraPass; the engine shows the active scene's flagged RTs as "Surround View" thumbnails.
void VehicleScene::BuildSurroundViews()
{
    const float egoX = Vehicle::SyntheticSource::kEgoLaneX;
    const float eyeY = 1.3f;

    // Match the main view's uniform grey studio background. Since the environment is a flat grey,
    // clearing to that grey is identical to drawing the grey skybox; the thin lane lines only need
    // enough RT resolution not to fall below a pixel (512² vs the old 256²).
    const float kBg[3] = { 0.66f, 0.66f, 0.67f };

    // One perspective surround camera + square RT, looking outward from the ego edge.
    auto addSurround = [&](const char* name, ZNVector3 pos, float pitchDeg, float yawDeg)
    {
        auto* rt = new RenderTexture();
        rt->Init(512, 512);
        rt->SetClearColor(kBg[0], kBg[1], kBg[2]);

        auto* cam = new ZNCamera();
        cam->SetPosition(pos);
        cam->SetRotation(pitchDeg, yawDeg);
        cam->SetPerspective(3.14159265f / 2.0f, 1.0f, 0.1f, 140.0f); // 90deg, square RT

        surroundViews.push_back({ name, cam, rt });
        AddOffscreenCamera(cam, rt, name, offscreenShader);
    };

    addSurround("Front", ZNVector3(egoX,        eyeY,  2.8f), -10.0f,   0.0f); // +Z
    addSurround("Rear",  ZNVector3(egoX,        eyeY, -2.8f), -10.0f, 180.0f); // -Z
    // Steeper pitch than Front/Rear: looking near-side-on with a shallow tilt put most of the frame
    // in the flat grey background (no skybox in this scene) with only a thin strip of road at the
    // bottom. Tilting further down fills the frame with pavement/lane markings instead.
    addSurround("Left",  ZNVector3(egoX - 1.3f, eyeY,  0.0f), -35.0f, -90.0f); // -X
    addSurround("Right", ZNVector3(egoX + 1.3f, eyeY,  0.0f), -35.0f,  90.0f); // +X

    // Bird's-eye: orthographic, straight down. SetView (explicit up) sidesteps the pitch=-90
    // gimbal in the pitch/yaw path; image "up" = +Z = ego forward.
    {
        auto* rt = new RenderTexture();
        rt->Init(512, 512);
        rt->SetClearColor(kBg[0], kBg[1], kBg[2]);

        auto* cam = new ZNCamera();
        cam->SetOrthographic(48.0f, 48.0f, 0.1f, 120.0f);
        cam->SetView(ZNVector3(egoX, 45.0f, 14.0f),  // eye high above the road ahead of ego
                     ZNVector3(egoX,  0.0f, 14.0f),  // look straight down
                     ZNVector3(0.0f,  0.0f, 1.0f));  // +Z up in the image

        surroundViews.push_back({ "Top-Down", cam, rt });
        AddOffscreenCamera(cam, rt, "Top-Down", offscreenShader);
    }
}

// ---- per-frame -------------------------------------------------------------------------------

void VehicleScene::Update(float deltaTime)
{
    ZNScene::Update(deltaTime);

    if (dataSource)
    {
        const Vehicle::FrameData* shown = nullptr;
        if (scrubbing)
        {
            // Timeline was dragged: show the exact log frame at the seek position, and drop the
            // interpolator's buffer so playback resumes cleanly (no interp across the discontinuity).
            interpolator.Reset();
            shown = &dataSource->GetCurrentFrame();
            scrubbing = false;
        }
        else
        {
            // Interpolator ticks the source at the sensor cadence, then resamples to this render frame.
            interpolator.Update(deltaTime);
            shown = &interpolator.Sample();
        }
        binding.Apply(*shown, *this);

        // Lane dashes flow toward the ego at its forward speed (from the shown frame; unless paused).
        const float scroll = dataSource->IsPaused()
            ? 0.0f
            : shown->ego.speed * deltaTime * dataSource->GetSpeed();
        UpdateLaneDashes(scroll);

        TickRecording();   // capture new sensor frames if a live recording is running
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

    // One merged panel (Surround View + DataSource), same "sections in one window" style as the
    // Outliner/Inspector panel. Anchored top-right, stacked directly below the engine's GBuffer
    // Preview panel (whatever its current height -- collapsed or expanded).
    const float gbufferH = GetWindowHeightByName("GBuffer Preview");
    const float topMargin = 10.0f + (gbufferH > 0.0f ? gbufferH + 10.0f : 0.0f);
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - 10.0f, vp->WorkPos.y + topMargin),
                             ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(330.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Vehicle");

    // --- Class legend: color -> class, so a first-time viewer doesn't have to guess the code. ---
    if (ImGui::CollapsingHeader("Legend", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto swatch = [](const char* label, ZNVector4 color)
        {
            ImGui::ColorButton((std::string("##") + label).c_str(), ImVec4(color.x, color.y, color.z, 1.0f),
                                ImGuiColorEditFlags_NoTooltip, ImVec2(14.0f, 14.0f));
            ImGui::SameLine();
            ImGui::TextUnformatted(label);
        };
        swatch("Ego",        ZNVector4(0.75f, 0.06f, 0.05f, 1.0f)); ImGui::SameLine(0.0f, 20.0f);
        swatch("Car",        ZNVector4(0.10f, 0.68f, 0.60f, 1.0f)); ImGui::SameLine(0.0f, 20.0f);
        swatch("Pedestrian", ZNVector4(0.95f, 0.45f, 0.35f, 1.0f));
        swatch("Cyclist",    ZNVector4(0.95f, 0.62f, 0.15f, 1.0f));
    }

    // --- Surround View section (multi-camera, stage 4) ---
    // Turn each surround RT into an ImGui thumbnail via the shared ImGui layer (same machinery as
    // the engine's GBuffer preview). Slots 1-6 are the GBuffer channels, so start at 7.
    if (!surroundViews.empty() &&
        ImGui::CollapsingHeader("Surround View", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto* cq  = GraphicsContext::GetInstance().GetAs<CommandQueue>();
        auto* gui = cq ? cq->GetImGuiLayer() : nullptr;
        auto* dev = static_cast<GraphicsDevice*>(GraphicsContext::GetInstance().GetDevice());
        if (gui && dev)
        {
            const float cell = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
            int slot = 7;
            for (size_t i = 0; i < surroundViews.size(); ++i)
            {
                const SurroundView& sv = surroundViews[i];
                ImTextureID tex = gui->SetTexture(dev->Device().Get(), sv.rt->GetSRVCpuHandle(), slot++);
                ImGui::BeginGroup();
                ImGui::TextUnformatted(sv.name.c_str());
                ImGui::Image(tex, ImVec2(cell, cell));
                ImGui::EndGroup();
                const bool secondInRow = (i % 2 == 1);
                const bool lastItem    = (i + 1 == surroundViews.size());
                if (!secondInRow && !lastItem) ImGui::SameLine();
            }
        }
    }

    // --- DataSource section ---
    if (!ImGui::CollapsingHeader("DataSource", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::End();
        return;
    }

    const Vehicle::FrameData& frame = dataSource->GetCurrentFrame();

    // Source switch: Live synthetic vs recorded log — same seam, scene/binding unchanged (stage 5).
    ImGui::Text("Source: %s", dataSource->GetName());
    ImGui::SameLine();

    const bool isLive = (dataSource == synthetic.get());

    // Lock the source switches while recording so the capture isn't cut off mid-stream.
    ImGui::BeginDisabled(recording);
    if (ImGui::SmallButton("Live"))
        UseSource(synthetic.get());
    ImGui::SameLine();
    if (ImGui::SmallButton("Load Log"))
    {
        if (!logSource) logSource = std::make_unique<Vehicle::LogPlaybackSource>();
        if (logSource->Load(GetResourcePath() / L"Logs" / L"scenario.json"))
            UseSource(logSource.get());
    }
    ImGui::EndDisabled();

    // Record captures the LIVE stream forward from now (only meaningful on the synthetic source).
    ImGui::SameLine();
    if (recording)
    {
        const float elapsed = recordBuffer.empty() ? 0.0f : (recordLastTs - recordFirstTs);
        char lbl[48];
        std::snprintf(lbl, sizeof(lbl), "Stop %.1f/%.0fs###rec", elapsed, recordTarget);
        if (ImGui::SmallButton(lbl))
            FinishRecording();
    }
    else
    {
        ImGui::BeginDisabled(!isLive);
        if (ImGui::SmallButton("Record 20s###rec"))
            StartRecording();
        ImGui::EndDisabled();
    }
    if (!recordStatus.empty())
        ImGui::TextDisabled("%s", recordStatus.c_str());

    bool paused = dataSource->IsPaused();
    if (ImGui::Button(paused ? "Play" : "Pause"))
        dataSource->SetPaused(!paused);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);
    float spd = dataSource->GetSpeed();
    if (ImGui::SliderFloat("Speed", &spd, 0.25f, 4.0f, "%.2fx"))
        dataSource->SetSpeed(spd);

    // Log timeline (seek) — deterministic scrubbing a procedural source can't give (mockup timeline).
    if (dataSource == logSource.get() && logSource && logSource->IsLoaded())
    {
        float ph = logSource->GetPlayhead();
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::SliderFloat("##timeline", &ph, 0.0f, logSource->GetDuration(), "t = %.2f s"))
        {
            logSource->SetPlayhead(ph);
            scrubbing = true;   // Update() shows this exact frame and resets the interpolator
        }
        bool lp = logSource->IsLoop();
        if (ImGui::Checkbox("Loop", &lp)) logSource->SetLoop(lp);
        ImGui::SameLine();
        ImGui::Text("%d frames @ %.0f Hz", logSource->FrameCount(), logSource->GetSensorHz());
    }
    else
    {
        ImGui::Text("t = %.1f s", frame.timestamp);
    }

    ImGui::Separator();
    ImGui::Text("Ego speed : %.1f m/s (%.0f km/h)", frame.ego.speed, frame.ego.speed * 3.6f);
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
