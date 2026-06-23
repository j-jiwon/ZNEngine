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

    struct SpotLightDebug {
        ZNFramework::ZNGameObject* marker = nullptr;
        ZNFramework::ZNGameObject* cone = nullptr;
        ZNFramework::ZNMaterial* markerMaterial = nullptr;
        ZNFramework::ZNMaterial* coneMaterial = nullptr;
    };

    enum class SelectedType { None, GameObject, SpotLight, DirectionalLight, Camera };
    struct Selection {
        SelectedType type = SelectedType::None;
        void* ptr = nullptr;
    };

private:
    void ToggleDebugVisuals();

    // Shaders
    ZNFramework::ZNShader* defaultShader = nullptr;
    ZNFramework::ZNShader* gridShader = nullptr;
    ZNFramework::ZNShader* debugConeShader = nullptr;

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
        SpotLightDebug spotLight1;
        SpotLightDebug spotLight2;
        ZNFramework::ZNGameObject* gridPlane = nullptr;
        ZNFramework::ZNMaterial* gridMaterial = nullptr;
        ZNFramework::ZNGameObject* cameraMarker = nullptr;
        ZNFramework::ZNGameObject* cameraLens   = nullptr;
        ZNFramework::ZNMaterial*   cameraMarkerMaterial = nullptr;
        bool showGrid       = false;
        bool showSpotLights = false;
        bool showCameras    = false;
    } debug;

    // Interactive state
    ZNFramework::ZNGameObject* turntableObj = nullptr;
    bool turntableEnabled = false;

    // CCTV multi-camera demo
    struct CCTVSetup {
        ZNFramework::ZNCamera*      camera    = nullptr;
        ZNFramework::RenderTexture* rt        = nullptr;
        ZNFramework::ZNShader*      fwdShader = nullptr; // forward_unlit for CCTV view
        ZNFramework::ZNMaterial*    floorMat  = nullptr;
        ZNFramework::ZNMaterial*    cubeMat   = nullptr;
        ZNFramework::ZNMaterial*    sphereMat = nullptr;
        ZNFramework::ZNMaterial*    bunnyMat  = nullptr;
        ZNFramework::ZNMaterial*    tvMat     = nullptr; // deferred mat w/ CCTV RT as albedo
        ZNFramework::ZNGameObject*  tvObj     = nullptr;
    } cctv;

    // ImGui state
    ZNFramework::ZNDirectionalLight* dirLight = nullptr;
    float fpsAccum = 0.0f;
    int   fpsFrames = 0;
    float fpsDisplay = 0.0f;
    float cpuUsagePercent = 0.0f;
    Selection selection;
};
