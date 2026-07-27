#pragma once
#include "FrameData.h"
#include "ZNFramework/Scene/ZNObjectHandle.h"
#include <unordered_map>

class VehicleScene;

namespace Vehicle
{
    // FrameData -> scene graph: update matched tracks (by id), adopt new ids, destroy vanished ones.
    // "pool reuse" = ZNScene recycles the SLOT (stable handle + stale detection), not the object
    // memory — spawn/despawn really alloc/free. Instance pooling would be a stage-6 optimisation.
    class SceneBinding
    {
    public:
        void Apply(const FrameData& frame, VehicleScene& scene);

        int LiveTrackCount() const { return static_cast<int>(trackToHandle.size()); }

    private:
        // id -> pool handle. Handle over raw ptr so a freed/reused slot resolves to null, not a dangler.
        std::unordered_map<int, ZNFramework::ZNObjectHandle> trackToHandle;
    };
}
