#pragma once
#include <ZNFramework.h>

namespace ZNFramework { class RenderTexture; }

// Surveillance-room demo: overhead CCTV camera watches monitored objects,
// feeds into a TV screen the player stands in front of.
class CCTVScene : public ZNFramework::ZNScene
{
public:
    CCTVScene()  = default;
    ~CCTVScene() = default;

    void Initialize()             override;
    void Update(float deltaTime)  override;
    void Render()                 override;
    void RenderForward()          override;

private:
    ZNFramework::ZNShader* defaultShader  = nullptr;
    ZNFramework::ZNShader* cctvShader    = nullptr; // forward_lit for offscreen pass
    ZNFramework::ZNShader* tvUnlitShader = nullptr; // screen_unlit for TV display

    // Objects visible in the main view and monitored by CCTV
    ZNFramework::ZNGameObject* floor   = nullptr;
    ZNFramework::ZNGameObject* boxA    = nullptr;
    ZNFramework::ZNGameObject* boxB    = nullptr;
    ZNFramework::ZNGameObject* boxC    = nullptr;
    ZNFramework::ZNGameObject* sphere  = nullptr;
    ZNFramework::ZNMaterial*   floorMat  = nullptr;
    ZNFramework::ZNMaterial*   boxAMat   = nullptr;
    ZNFramework::ZNMaterial*   boxBMat   = nullptr;
    ZNFramework::ZNMaterial*   boxCMat   = nullptr;
    ZNFramework::ZNMaterial*   sphereMat = nullptr;

    // CCTV infrastructure
    ZNFramework::ZNCamera*      cctvCamera = nullptr;
    ZNFramework::RenderTexture* cctvRT     = nullptr;
    ZNFramework::ZNGameObject*  tvScreen   = nullptr;
    ZNFramework::ZNMaterial*    tvMat      = nullptr;

    // Room model (loaded from FBX)
    struct RoomModel {
        std::vector<ZNFramework::ZNGameObject*> objects;
        std::vector<ZNFramework::ZNMaterial*>   materials; // main (deferred) materials
    } room;

};
