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

    // Scene objects (always visible)
    struct SceneObjects {
        ZNFramework::ZNGameObject* floor = nullptr;
        ZNFramework::ZNGameObject* cube = nullptr;
        ZNFramework::ZNGameObject* sphere = nullptr;
        ZNFramework::ZNMaterial* floorMaterial = nullptr;
        ZNFramework::ZNMaterial* cubeMaterial = nullptr;
        ZNFramework::ZNMaterial* sphereMaterial = nullptr;
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

    // CCTV multi-camera demo
    struct CCTVSetup {
        ZNFramework::ZNCamera*      camera       = nullptr;
        ZNFramework::RenderTexture* rt           = nullptr;
        ZNFramework::ZNShader*      fwdShader    = nullptr; // forward_pbr for CCTV offscreen view
        ZNFramework::ZNShader*      tvUnlitShader= nullptr; // screen_unlit for TV display
        ZNFramework::ZNMaterial*    tvMat        = nullptr;
        ZNFramework::ZNGameObject*  tvObj        = nullptr;
    } cctv;

};
