#pragma once
#include <ZNFramework.h>
#include "Vehicle/SyntheticSource.h"
#include "Vehicle/FrameInterpolator.h"
#include "Vehicle/SceneBinding.h"
#include <vector>
#include <memory>

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
    void UpdateLaneDashes(float scrollDelta);
    void RenderDataSourcePanel(); // mockup: bottom DataSource bar + tracked/latency stats

    ZNFramework::ZNShader* mainShader = nullptr;

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

    std::unique_ptr<Vehicle::SyntheticSource> dataSource;
    Vehicle::FrameInterpolator                interpolator;   // resamples 30Hz source -> render fps
    Vehicle::SceneBinding                     binding;
};
