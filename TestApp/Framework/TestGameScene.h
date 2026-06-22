#pragma once
#include <ZNFramework.h>
#include <vector>

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

    enum class SelectedType { None, GameObject, SpotLight, DirectionalLight };
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

    // Debug visuals (toggle with F1)
    struct DebugVisuals {
        SpotLightDebug spotLight1;
        SpotLightDebug spotLight2;
        ZNFramework::ZNGameObject* gridPlane = nullptr;
        ZNFramework::ZNMaterial* gridMaterial = nullptr;
        bool visible = false;
    } debug;

    // Interactive state
    ZNFramework::ZNGameObject* turntableObj = nullptr;
    bool turntableEnabled = false;

    // ImGui state
    ZNFramework::ZNDirectionalLight* dirLight = nullptr;
    float fpsAccum = 0.0f;
    int   fpsFrames = 0;
    float fpsDisplay = 0.0f;
    float cpuUsagePercent = 0.0f;
    Selection selection;
};
