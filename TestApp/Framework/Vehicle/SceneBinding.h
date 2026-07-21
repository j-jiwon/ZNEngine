#pragma once
#include "FrameData.h"
#include "ZNFramework/Scene/ZNObjectHandle.h"
#include <unordered_map>

class VehicleScene;

namespace Vehicle
{
    // Binds a stream of FrameData onto the scene graph: the generic layer the roadmap sells as the
    // headline design point. Each frame it reconciles the frame's tracked objects against the live
    // game objects — updates matched tracks (by id), spawns new ones from the scene's pool, and
    // destroys tracks that vanished. Track id == pool identity, one adapter, source-agnostic.
    class SceneBinding
    {
    public:
        void Apply(const FrameData& frame, VehicleScene& scene);

        int LiveTrackCount() const { return static_cast<int>(trackToHandle.size()); }

    private:
        // id -> stable pool handle. Handles (not raw pointers) because tracks outlive frames and the
        // pool may reuse slots; Resolve() turns a stale handle into null instead of a dangler.
        std::unordered_map<int, ZNFramework::ZNObjectHandle> trackToHandle;
    };
}
