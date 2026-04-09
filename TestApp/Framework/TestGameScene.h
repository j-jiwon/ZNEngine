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

    ZNFramework::ZNShader* GetDefaultShader() const { return defaultShader; }

private:
    // Resources
    ZNFramework::ZNShader* defaultShader = nullptr;

    // FBX Models
    std::vector<ZNFramework::ZNGameObject*> modelObjects;
    std::vector<ZNFramework::ZNMaterial*> materials;
    std::vector<ZNFramework::ZNTexture*> textures;

    // Debug visualization
    ZNFramework::ZNGameObject* crosshair = nullptr;
    ZNFramework::ZNGameObject* lightIndicator = nullptr;
    ZNFramework::ZNGameObject* axisX = nullptr;
    ZNFramework::ZNGameObject* axisY = nullptr;
    ZNFramework::ZNGameObject* axisZ = nullptr;
    ZNFramework::ZNMaterial* debugMaterial = nullptr;
    ZNFramework::ZNMaterial* redMaterial = nullptr;
    ZNFramework::ZNMaterial* greenMaterial = nullptr;
    ZNFramework::ZNMaterial* blueMaterial = nullptr;
};
