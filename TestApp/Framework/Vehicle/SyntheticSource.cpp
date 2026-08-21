#include "SyntheticSource.h"

namespace Vehicle
{
    namespace
    {
        // One row per lane. The four driving lanes reuse SyntheticSource::kLaneX so the scene's lane
        // markings and the traffic stay in lockstep. 차선1,2 are oncoming; 차선3 (ego) carries only
        // pacing cars (ego speed, fixed ahead) so nothing passes through the immovable ego; 차선4 is
        // same-direction. Cyclists/pedestrians sit on the shoulders beyond the outer lane lines.
        struct LaneDef
        {
            float       x;
            float       speed;   // absolute m/s along +Z (negative = oncoming)
            int         count;   // agents in this lane (the per-lane visible cap)
            ObjectClass cls;
            float       bboxW, bboxL;
            bool        pacing;  // ego-lane cars: hold a fixed offset ahead, never move/recycle
        };

        const LaneDef kLanes[] = {
            { SyntheticSource::kLaneX[0], -12.0f, 3, ObjectClass::Car,        1.9f, 4.3f, false }, // 차선1 oncoming
            { SyntheticSource::kLaneX[1], -16.0f, 3, ObjectClass::Car,        1.9f, 4.3f, false }, // 차선2 oncoming
            { SyntheticSource::kLaneX[2], SyntheticSource::kEgoSpeed, 2, ObjectClass::Car, 1.9f, 4.3f, true }, // 차선3 ego (pacing)
            { SyntheticSource::kLaneX[3],  18.0f, 4, ObjectClass::Car,        1.9f, 4.3f, false }, // 차선4 same-dir
            {  9.0f,   4.2f, 1, ObjectClass::Cyclist,    0.7f, 1.8f, false },                      // right shoulder
            { -9.0f,  -4.2f, 1, ObjectClass::Cyclist,    0.7f, 1.8f, false },                      // left shoulder
            { 10.5f,   0.0f, 1, ObjectClass::Pedestrian, 0.6f, 0.6f, false },                      // right sidewalk
            {-10.5f,   0.0f, 1, ObjectClass::Pedestrian, 0.6f, 0.6f, false },                      // left sidewalk
        };
        constexpr int kLaneCount = static_cast<int>(sizeof(kLanes) / sizeof(kLanes[0]));
    }

    SyntheticSource::SyntheticSource(unsigned seed)
        : rng(seed), seedValue(seed)
    {
        SpawnAgents();
    }

    // Stress knob (see the header). Rebuilding from scratch rather than adding/removing agents keeps
    // the lane spacing correct at every multiplier, and every track gets a fresh id, so the binding
    // sees a clean despawn-everything/spawn-everything -- no stale objects at the old density.
    void SyntheticSource::SetDensityMultiplier(int m)
    {
        if (m < 1) m = 1;
        if (m == density) return;
        density = m;
        SpawnAgents();
    }

    void SyntheticSource::SpawnAgents()
    {
        rng.seed(seedValue);
        std::uniform_real_distribution<float> unit(0.f, 1.f);
        const float range = zFront - zBack;

        agents.clear();
        for (int li = 0; li < kLaneCount; ++li)
        {
            const LaneDef& L = kLanes[li];
            // The ego lane's pacing cars hold fixed slots ahead of the ego; multiplying them would
            // just stack cars kilometres down the road, so density applies to moving traffic only.
            const int count = L.pacing ? L.count : L.count * density;
            for (int k = 0; k < count; ++k)
            {
                Agent a;
                a.lane  = li;
                a.cls   = L.cls;
                a.relX  = L.x;
                a.speed = L.speed;
                a.bboxW = L.bboxW;
                a.bboxL = L.bboxL;
                a.id    = nextId++;
                a.relZ  = L.pacing
                    ? 18.0f + k * 24.0f                             // fixed slots ahead of the ego
                    : zBack + (k + unit(rng)) * (range / count);    // one car per even slice of the lane
                // At x1 the lane is a single file of evenly spaced cars, as before. Denser lanes
                // would otherwise be a line of coincident cars, so spread them across the lane.
                if (density > 1 && !L.pacing)
                    a.relX += (unit(rng) - 0.5f) * kLaneWidth * 0.8f;
                agents.push_back(a);
            }
        }
        BuildFrame();
    }

    void SyntheticSource::Update(float dt)
    {
        // Pure "advance by dt" generator; speed/pause are applied by the clock driver (FrameInterpolator).
        frame.timestamp += dt;

        const float range = zFront - zBack;
        for (auto& a : agents)
        {
            if (kLanes[a.lane].pacing) continue;   // ego-lane pacing cars hold their offset

            a.relZ += (a.speed - egoSpeed) * dt;   // move relative to the ego
            // Ran off an end -> re-enter at the far end with a NEW id, so the binding sees a genuine
            // despawn + spawn. Both ends are off-camera, so the recycle itself is never seen.
            if (a.relZ > zFront)     { a.relZ -= range; a.id = nextId++; }
            else if (a.relZ < zBack) { a.relZ += range; a.id = nextId++; }
        }

        BuildFrame();
    }

    void SyntheticSource::BuildFrame()
    {
        frame.ego = EgoState{ egoSpeed, 0.f, 0.f };
        frame.objects.clear();
        frame.objects.reserve(agents.size());
        for (const auto& a : agents)
        {
            TrackedObject o;
            o.id       = a.id;
            o.cls      = a.cls;
            o.relX     = a.relX;
            o.relZ     = a.relZ;
            o.relSpeed = a.speed - egoSpeed;
            o.bboxW    = a.bboxW;
            o.bboxL    = a.bboxL;
            frame.objects.push_back(o);
        }
    }
}
