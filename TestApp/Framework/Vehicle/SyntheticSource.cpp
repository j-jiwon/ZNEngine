#include "SyntheticSource.h"
#include <algorithm>
#include <cmath>

namespace Vehicle
{
    SyntheticSource::SyntheticSource(int agentCount, unsigned seed)
        : rng(seed)
    {
        agents.resize(agentCount > 0 ? agentCount : 1);

        // Spread the initial fleet across the whole range instead of all sitting on a boundary.
        std::uniform_real_distribution<float> zDist(zMin, zMax);
        for (auto& a : agents)
        {
            Respawn(a);
            a.relZ = zDist(rng);
        }
    }

    void SyntheticSource::Respawn(Agent& a)
    {
        std::uniform_real_distribution<float> unit(0.f, 1.f);
        const float r = unit(rng);

        a.id = nextId++;

        // Lanes are discrete (no X jitter) so "same lane" is exact — the overlap solver groups by
        // lane X, and cars in a lane hold a following gap instead of passing through each other.
        if (r < 0.45f)                    // same-direction car (right-hand lanes, incl. ego's lane)
        {
            a.cls   = ObjectClass::Car;
            a.relX  = (unit(rng) < 0.5f ? kEgoLaneX : 5.5f);
            a.speed = egoSpeed + (unit(rng) - 0.5f) * 10.0f;   // +/- 5 m/s around ego
            a.bboxW = 1.9f; a.bboxL = 4.3f;
        }
        else if (r < 0.70f)               // oncoming car (left-hand lanes)
        {
            a.cls   = ObjectClass::Car;
            a.relX  = (unit(rng) < 0.5f ? -2.0f : -5.5f);
            a.speed = -egoSpeed * (0.7f + unit(rng) * 0.6f);   // heading the other way
            a.bboxW = 1.9f; a.bboxL = 4.3f;
        }
        else if (r < 0.88f)               // cyclist (roadside, slow)
        {
            a.cls   = ObjectClass::Cyclist;
            a.relX  = (unit(rng) < 0.5f ? 7.5f : -7.5f);
            a.speed = egoSpeed * 0.30f * (a.relX > 0.f ? 1.f : -1.f);
            a.bboxW = 0.7f; a.bboxL = 1.8f;
        }
        else                              // pedestrian (near sidewalk, near-still)
        {
            a.cls   = ObjectClass::Pedestrian;
            a.relX  = (unit(rng) < 0.5f ? 8.8f : -8.8f);
            a.speed = (unit(rng) - 0.5f) * 2.0f;
            a.bboxW = 0.6f; a.bboxL = 0.6f;
        }

        // Enter from the end it moves away from: rate<=0 (relZ shrinking) -> front (zMax), else back.
        const float rate = a.speed - egoSpeed;
        a.relZ = (rate <= 0.f) ? zMax : zMin;
    }

    void SyntheticSource::Update(float dt)
    {
        if (paused)
            return;

        dt *= speed;
        frame.timestamp += dt;

        for (auto& a : agents)
        {
            a.relZ += (a.speed - egoSpeed) * dt;
            if (a.relZ < zMin || a.relZ > zMax)
                Respawn(a);
        }

        ResolveOverlaps();   // keep a following gap; no car passes through another (or the ego)

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

    void SyntheticSource::ResolveOverlaps()
    {
        // Within each lane (exact X), no two bodies may sit closer than their half-lengths + a gap.
        // A few relaxation passes push overlapping pairs apart along Z; the ego is an immovable
        // anchor in its lane, so cars yield to it, never the reverse. Cars keep their own speeds —
        // they just can't occupy the same space, which reads as "sped up / slowed to follow".
        const float kGap   = 1.2f;                       // metres of clear space between bodies
        const int   egoKey = static_cast<int>(std::lround(kEgoLaneX * 10.0f));

        struct Node { float* z; float half; bool movable; };

        // Distinct lane keys present this frame (handful of lanes -> linear scan is fine).
        std::vector<int> keys;
        for (const auto& a : agents)
        {
            const int k = static_cast<int>(std::lround(a.relX * 10.0f));
            if (std::find(keys.begin(), keys.end(), k) == keys.end())
                keys.push_back(k);
        }

        for (const int key : keys)
        {
            float egoZ = 0.0f;   // ego sits at relZ 0 in its own lane (origin), immovable
            std::vector<Node> lane;
            for (auto& a : agents)
                if (static_cast<int>(std::lround(a.relX * 10.0f)) == key)
                    lane.push_back({ &a.relZ, a.bboxL * 0.5f, true });
            if (key == egoKey)
                lane.push_back({ &egoZ, kEgoLen * 0.5f, false });

            if (lane.size() < 2)
                continue;

            for (int pass = 0; pass < 4; ++pass)
            {
                std::sort(lane.begin(), lane.end(),
                          [](const Node& a, const Node& b) { return *a.z < *b.z; });
                for (size_t i = 0; i + 1 < lane.size(); ++i)
                {
                    Node& A = lane[i];
                    Node& B = lane[i + 1];
                    const float need = A.half + B.half + kGap;
                    const float overlap = need - (*B.z - *A.z);
                    if (overlap <= 0.0f)
                        continue;

                    if (A.movable && B.movable) { *A.z -= overlap * 0.5f; *B.z += overlap * 0.5f; }
                    else if (A.movable)         { *A.z -= overlap; }
                    else if (B.movable)         { *B.z += overlap; }
                }
            }
        }
    }
}
