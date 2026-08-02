#pragma once
#include <ZNFramework.h>
#include "Vehicle/SyntheticSource.h"
#include "Vehicle/LogPlaybackSource.h"
#include "Vehicle/FrameInterpolator.h"
#include "Vehicle/SceneBinding.h"
#include <vector>
#include <memory>
#include <string>

namespace ZNFramework { class RenderTexture; }

// Automotive 3D-viz demo. Ego fixed at the origin; a data source emits ego-relative FrameData each
// tick, SceneBinding maps it onto pool objects. Engine primitives, colour-coded by class
// (ego=blue, car=teal, ped=coral). See .claude/zn_automative3d.md.
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

private:
    void BuildClassResources();
    void BuildStaticStage();     // ground plane, ego box, scrolling lane dashes
    void BuildSurroundViews();   // top-down + 4-way surround offscreen cameras (stage 4)
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

    struct ClassRes { ZNFramework::ZNMesh* mesh = nullptr; ZNFramework::ZNMaterial* mat = nullptr; };
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
