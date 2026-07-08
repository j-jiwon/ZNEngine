#pragma once
#include <ZNFramework.h>

class MirrorBallScene : public ZNFramework::ZNScene
{
public:
    MirrorBallScene() = default;
    ~MirrorBallScene() = default;

    void Initialize()            override;
    void Update(float deltaTime) override;
    void Render()                override;
    void RenderForward()         override;

private:
    ZNFramework::ZNShader* defaultShader     = nullptr;
    ZNFramework::ZNShader* glassShader       = nullptr;
    ZNFramework::ZNShader* envCaptureShader  = nullptr; // forward_pbr.hlsli, used only for the env cubemap capture

    struct BallModel {
        std::vector<ZNFramework::ZNGameObject*> objects;
        std::vector<ZNFramework::ZNMaterial*>   materials;
    };
    BallModel mirrorBall;  // Metallic 1.0 / Roughness 0.0, deferred pass
    BallModel glassBall;   // Translucent glass, forward pass

    struct MonsterModel {
        std::vector<ZNFramework::ZNGameObject*> objects;
        std::vector<ZNFramework::ZNMaterial*>   materials; // one per glTF material slot
    };
    MonsterModel monster;  // Monster_S_0.glb, deferred pass

    struct RoomModel {
        std::vector<ZNFramework::ZNGameObject*> objects;
        std::vector<ZNFramework::ZNMaterial*>   materials; // one per glTF material slot
    };
    RoomModel room;  // room.glb background, deferred pass

    ZNFramework::ZNSpotLight*  spotLights[4] = {};
    ZNFramework::ZNPointLight* innerLight    = nullptr;
};
