#include "SyntheticSource.h"

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

        if (r < 0.45f)                    // same-direction car (right-hand lanes)
        {
            a.cls   = ObjectClass::Car;
            a.relX  = (unit(rng) < 0.5f ? 2.0f : 5.5f) + (unit(rng) - 0.5f) * 0.6f;
            a.speed = egoSpeed + (unit(rng) - 0.5f) * 10.0f;   // +/- 5 m/s around ego
            a.bboxW = 1.9f; a.bboxL = 4.3f;
        }
        else if (r < 0.70f)               // oncoming car (left-hand lanes)
        {
            a.cls   = ObjectClass::Car;
            a.relX  = -((unit(rng) < 0.5f ? 2.0f : 5.5f) + (unit(rng) - 0.5f) * 0.6f);
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
            a.relX  = (unit(rng) < 0.5f ? 8.8f : -8.8f) + (unit(rng) - 0.5f);
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
