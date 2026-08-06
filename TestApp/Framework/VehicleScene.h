#pragma once
#include <ZNFramework.h>
#include "Vehicle/SyntheticSource.h"
#include "Vehicle/LogPlaybackSource.h"
#include "Vehicle/FrameInterpolator.h"
#include "Vehicle/SceneBinding.h"
#include <vector>
#include <memory>
#include <string>
#include <filesystem>

namespace ZNFramework { class RenderTexture; }

// Automotive 3D-viz demo. Ego fixed at the origin; a data source emits ego-relative FrameData each
// tick, SceneBinding maps it onto pool objects. Cars (ego + Car-class tracks) share one low-poly
// model, ego reskinned red; pedestrians/cyclists stay engine-primitive cubes (coral/amber).
// See .claude/zn_automative3d.md.
class VehicleScene : public ZNFramework::ZNScene
{
public:
    VehicleScene()  = default;
    ~VehicleScene() override;

    void Initialize()            override;
    void Update(float deltaTime) override;
    void Render()                override;
    void RenderForward()         override;

    // One shared mesh+material per class; spawned tracks reuse it and only differ by transform
    // (no per-object GPU buffer; material is mesh-bound, so no ambiguity — see ZNGameObject::Render).
    ZNFramework::ZNMesh*     GetClassMesh(Vehicle::ObjectClass c) const;
    ZNFramework::ZNMaterial* GetClassMaterial(Vehicle::ObjectClass c) const;

    // Car model (shared by ego + Car-class tracks) -- SceneBinding uses these to spawn/place tracks
    // when the model loaded, and falls back to a plain cube (GetClassMesh/Material) when it didn't.
    bool  HasCarModel()      const { return carModel.valid; }
    float GetCarFitScale()   const { return carModel.fitScale; }
    float GetCarGroundLift() const { return carModel.groundLift; }
    float GetCarForwardYaw() const { return carModel.modelForwardYaw; }
    ZNFramework::ZNObjectHandle SpawnCarTrack(const std::string& name);

    // Spawns a pedestrian/cyclist as root + body + head (+ frame bar for cyclist) children sharing
    // classRes' meshes — same "shared mesh, N instances" pattern as SpawnCarInstance. Called by
    // SceneBinding for every new Pedestrian/Cyclist track.
    ZNFramework::ZNObjectHandle SpawnHumanoidInstance(Vehicle::ObjectClass cls, const std::string& name, const std::string& tag);

private:
    void BuildClassResources();
    void BuildStaticStage();     // ground plane, ego car, scrolling lane dashes
    void BuildSurroundViews();   // top-down + 4-way surround offscreen cameras (stage 4)

    // A loaded low-poly car (multi-mesh, colour-per-material, no textures): shared meshes/materials
    // + a fit transform mapping model units -> metres. Instances share these and only differ by root.
    struct CarModel {
        std::vector<ZNFramework::ZNMesh*>     meshes;
        std::vector<ZNFramework::ZNMaterial*> mats;
        std::vector<int>                      meshMat;         // material index per mesh
        float fitScale        = 1.0f;   // uniform scale so the longest horizontal extent == targetLen
        float groundLift      = 0.0f;   // world Y that puts the model's underside on the ground
        float modelForwardYaw = 0.0f;   // deg added so the model faces +Z (travel direction)
        bool  valid           = false;
    };
    bool LoadCarModel(const std::filesystem::path& path, float targetLen, CarModel& out, bool isEgo = false);
    // Spawns a shared-mesh car instance (root + child meshes); caller sets root position/rotation.
    ZNFramework::ZNObjectHandle SpawnCarInstance(const CarModel& car, const std::string& name, const std::string& tag);
    // Repaints egoCarModel.mats[0] (the body shell) red, restoring every other material to its loaded
    // colour (from egoBaseMatParams). Material is mesh-bound (ZNGameObject::Render), so this is how a
    // single shared mesh gets a different colour.
    void ApplyEgoPaint();
    void UpdateLaneDashes(float scrollDelta);
    void RenderDataSourcePanel(); // mockup: bottom DataSource bar + tracked/latency stats
    void UseSource(Vehicle::IDataSource* src); // swap active source + re-init the interpolator

    // Live recording: capture the synthetic sensor stream to JSON, starting now (stage 5).
    void StartRecording();
    void FinishRecording();       // writes scenario.json, sets recordStatus
    void TickRecording();         // called each Update while recording: grab new sensor frames

    ZNFramework::ZNShader* mainShader      = nullptr;
    ZNFramework::ZNShader* offscreenShader = nullptr; // forward_pbr for the surround/top-down RTs

    ZNFramework::ZNGameObject* ego = nullptr;
    CarModel carModel;      // car_white, plain — shared by Car-class tracks
    CarModel egoCarModel;   // car_white, separate bake — one material gets painted red (ApplyEgoPaint)
    std::vector<ZNFramework::MaterialParams> egoBaseMatParams;  // egoCarModel.mats[i]'s loaded colour

    // Car: `mesh` is the plain-cube fallback used when no car model loaded. Pedestrian/Cyclist:
    // `mesh` is the body (a unit box, also reused for the cyclist's bike-frame bar child — same
    // material, just a different child transform) and `headMesh` the head — spawned as root+children
    // (SpawnHumanoidInstance) so they read as a person silhouette instead of a flat box.
    struct ClassRes {
        ZNFramework::ZNMesh* mesh     = nullptr;
        ZNFramework::ZNMaterial* mat  = nullptr;
        ZNFramework::ZNMesh* headMesh = nullptr;
    };
    ClassRes classRes[3];   // indexed by static_cast<int>(ObjectClass)

    // Scrolling lane dashes (convey ego forward motion while the ego stays at the origin).
    std::vector<ZNFramework::ZNGameObject*> laneDashes;
    std::vector<float>                      laneDashBaseZ;
    float laneCycle  = 0.f;   // wrap length = dashes-per-line * spacing
    float laneScroll = 0.f;   // metres scrolled so far
    float roadStartZ = -20.f; // lane dashes wrap into [roadStartZ, roadStartZ + laneCycle]

    // Meshes/materials aren't pool-owned — free them ourselves in the destructor.
    std::vector<ZNFramework::ZNMesh*>     ownedMeshes;
    std::vector<ZNFramework::ZNMaterial*> ownedMaterials;

    // Surround-view cameras + their render targets (not pool-owned — freed in the destructor).
    // Names label the thumbnails in the merged Vehicle panel.
    struct SurroundView {
        std::string              name;
        ZNFramework::ZNCamera*   cam = nullptr;
        ZNFramework::RenderTexture* rt = nullptr;
    };
    std::vector<SurroundView> surroundViews;

    // Two owned sources behind one seam; `dataSource` points at the active one (Live vs Log).
    std::unique_ptr<Vehicle::SyntheticSource>   synthetic;
    std::unique_ptr<Vehicle::LogPlaybackSource> logSource;
    Vehicle::IDataSource*                       dataSource = nullptr; // active (non-owning)

    Vehicle::FrameInterpolator                  interpolator;   // resamples sensor Hz -> render fps
    Vehicle::SceneBinding                       binding;
    bool scrubbing = false;   // timeline was dragged this frame (show exact log frame, skip interp)

    // Live recording state (Record button captures the live synthetic stream forward from "now").
    bool  recording      = false;
    float recordTarget   = 20.0f;   // seconds of sensor time to capture
    float recordFirstTs  = 0.0f;    // synthetic timestamp at capture start (re-base to 0)
    float recordLastTs   = -1.0f;   // last captured frame timestamp (dedup across render frames)
    std::vector<Vehicle::FrameData> recordBuffer;
    std::string recordStatus;       // last-write feedback shown in the panel
};
