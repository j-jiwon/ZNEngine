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
    void OnKeyboardEvent(const ZNFramework::KeyboardEvent& event) override;

    ZNFramework::ZNShader* GetDefaultShader() const { return defaultShader; }
    ZNFramework::ZNGameObject* GetTurntableObject() const { return turntableObj; }

private:
    // Toggle states
    bool turntableEnabled = true;
    bool planeVisible = true;
    bool lightEnabled = true;

    // Resources
    ZNFramework::ZNShader* defaultShader = nullptr;
    ZNFramework::ZNShader* gridShader = nullptr;

    // FBX Models
    std::vector<ZNFramework::ZNGameObject*> modelObjects;
    std::vector<ZNFramework::ZNMaterial*> materials;
    std::vector<ZNFramework::ZNTexture*> textures;

    // Debug visualization
    ZNFramework::ZNGameObject* turntableObj = nullptr;
    ZNFramework::ZNGameObject* crosshair = nullptr;
    ZNFramework::ZNGameObject* lightIndicator = nullptr;
    ZNFramework::ZNGameObject* plane = nullptr;

    ZNFramework::ZNGameObject* axisX = nullptr;
    ZNFramework::ZNGameObject* axisY = nullptr;
    ZNFramework::ZNGameObject* axisZ = nullptr;
    ZNFramework::ZNMaterial* debugMaterial = nullptr;
    ZNFramework::ZNMaterial* gridMaterial = nullptr;
    ZNFramework::ZNMaterial* redMaterial = nullptr;
    ZNFramework::ZNMaterial* greenMaterial = nullptr;
    ZNFramework::ZNMaterial* blueMaterial = nullptr;
};
