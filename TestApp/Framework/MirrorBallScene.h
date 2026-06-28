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
    ZNFramework::ZNShader* defaultShader = nullptr;
    ZNFramework::ZNShader* glassShader   = nullptr;

    struct BallModel {
        std::vector<ZNFramework::ZNGameObject*> objects;
        std::vector<ZNFramework::ZNMaterial*>   materials;
    };
    BallModel mirrorBall;  // Metallic 1.0 / Roughness 0.0, deferred pass
    BallModel glassBall;   // Translucent glass, forward pass

    ZNFramework::ZNSpotLight* spotLights[4] = {};
};
