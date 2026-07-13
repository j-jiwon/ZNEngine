#pragma once
#include <ZNFramework.h>
#include <vector>

// Forward declare D3D12-specific type to avoid pulling in platform headers here
namespace ZNFramework { class RenderTexture; }

class TestGameScene : public ZNFramework::ZNScene
{
public:
    TestGameScene() = default;
    ~TestGameScene() = default;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RenderForward() override;
    void OnKeyboardEvent(const ZNFramework::KeyboardEvent& event) override;

    ZNFramework::ZNShader* GetDefaultShader() const { return defaultShader; }
    ZNFramework::ZNGameObject* GetTurntableObject() const { return turntableObj; }

private:
    void ToggleDebugVisuals();

    // Shaders
    ZNFramework::ZNShader* defaultShader = nullptr;
    ZNFramework::ZNShader* gridShader = nullptr;

    // Loaded models
    struct ModelResources {
        std::vector<ZNFramework::ZNGameObject*> objects;
        std::vector<ZNFramework::ZNMaterial*> materials;
        std::vector<ZNFramework::ZNTexture*> textures;
    } models;

    // Scene objects (always visible): a PBR test grid of spheres — rows vary roughness
    // (row 0 = roughest 1.0, top, down to smoothest 0.0, bottom), columns vary metallic
    // (col 0 = non-metallic 0.0, left, up to fully metallic 1.0, right).
    struct SceneObjects {
        static constexpr int kGridRows = 5;
        static constexpr int kGridCols = 8;
        std::vector<ZNFramework::ZNGameObject*> spheres;
        std::vector<ZNFramework::ZNMaterial*> sphereMaterials; // one per (row,col), index = row*kGridCols+col
    } scene;

    // Debug visuals (toggle with F1, or per-item via Debug window)
    struct DebugVisuals {
        ZNFramework::ZNGameObject* gridPlane = nullptr;
        ZNFramework::ZNMaterial* gridMaterial = nullptr;
        bool showGrid = false;
    } debug;

    // Interactive state
    ZNFramework::ZNGameObject* turntableObj = nullptr;
    bool turntableEnabled = false;

};
