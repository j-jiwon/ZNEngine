#pragma once
#include "IDataSource.h"
#include <random>
#include <vector>

namespace Vehicle
{
    // Procedurally generates traffic around a fixed ego at the world origin. Each agent has an
    // absolute speed; its on-screen (relative) Z advances at (agentSpeed - egoSpeed). An agent
    // that leaves the sensor range is recycled to the far end WITH A NEW id, so the scene binding
    // observes a genuine despawn + spawn — this is what exercises the object pool (get/return).
    class SyntheticSource : public IDataSource
    {
    public:
        explicit SyntheticSource(int agentCount = 12, unsigned seed = 1337u);

        void             Update(float dt) override;
        const FrameData& GetCurrentFrame() const override { return frame; }
        const char*      GetName() const override { return "Synthetic"; }

        float GetEgoSpeed() const { return egoSpeed; }
        float GetSensorHz() const { return sensorHz; }

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

        // (Re)assign class/lane/speed and drop the agent at whichever end it will travel INTO,
        // so it converges through the sensor range instead of instantly re-exiting.
        void Respawn(Agent& a);

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
