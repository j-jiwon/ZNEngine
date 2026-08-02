#pragma once
#include "IDataSource.h"
#include <random>
#include <vector>

namespace Vehicle
{
    // Procedural traffic around a fixed ego at the origin. Each agent's relative Z advances at
    // (agentSpeed - egoSpeed); leaving the sensor range recycles it to the far end WITH A NEW id,
    // so the binding sees a genuine despawn + spawn (drives the pool's slot get/return).
    class SyntheticSource : public IDataSource
    {
    public:
        explicit SyntheticSource(int agentCount = 12, unsigned seed = 1337u);

        void             Update(float dt) override;
        const FrameData& GetCurrentFrame() const override { return frame; }
        const char*      GetName() const override { return "Synthetic"; }
        float            GetSensorHz() const override { return sensorHz; }

        // Ego occupies a real right-hand lane (not the centre line). The scene places the ego box +
        // chase camera here, and the overlap solver treats the ego as a fixed obstacle in this lane.
        static constexpr float kEgoLaneX = 2.0f;
        static constexpr float kEgoLen   = 4.4f;  // shared by the scene (box length) + overlap solver

    private:
        struct Agent
        {
            float       relX  = 0.f;
            float       relZ  = 0.f;
            float       speed = 0.f;                 // absolute m/s along +Z (negative = oncoming)
            ObjectClass cls   = ObjectClass::Car;
            float       bboxW = 1.8f;
            float       bboxL = 4.2f;
            int         id    = 0;
        };

        // (Re)assign class/lane/speed; drop it at the end it travels INTO so it doesn't re-exit at once.
        void Respawn(Agent& a);

        // Per-lane spacing: cars can change speed but never pass through each other (or the ego).
        // A light relaxation pass pushes any overlapping pair apart to a minimum following gap.
        void ResolveOverlaps();

        std::vector<Agent> agents;
        FrameData          frame;
        std::mt19937       rng;

        float egoSpeed = 14.0f;   // ~50 km/h
        float zMin     = -22.0f;  // behind ego (recycle boundary)
        float zMax     =  72.0f;  // ahead of ego (recycle boundary)
        float sensorHz = 30.0f;   // nominal sensor rate (drives the latency readout)
        int   nextId   = 1;
    };
}
