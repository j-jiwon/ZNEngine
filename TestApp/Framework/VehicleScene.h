#pragma once
#include <ZNFramework.h>
#include "Vehicle/SyntheticSource.h"
#include "Vehicle/SceneBinding.h"
#include <vector>
#include <memory>

// Automotive 3D-visualisation demo. The ego is fixed at the world origin; a data source emits a
// relative-coordinate FrameData every tick and SceneBinding maps it onto pool-owned game objects.
// Objects are engine primitives, colour-coded by class (ego=blue, car=teal, pedestrian=coral).
// See .claude/zn_automative3d.md (vertical slice) and automative_demo_layout_mockup.html.
class VehicleScene : public ZNFramework::ZNScene
{
public:
    VehicleScene()  = default;
    ~VehicleScene() override;

    void Initialize()            override;
    void Update(float deltaTime) override;
    void Render()                override;
    void RenderForward()         override;

    // Shared per-class render resources: one mesh (with its material bound) per class. Spawned
    // track objects reuse these — only the transform differs per object — so there is no per-object
    // GPU-buffer allocation and no material-sharing ambiguity (see ZNGameObject::Render).
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
    Vehicle::SceneBinding                     binding;
};
