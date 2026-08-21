#pragma once
#include "IDataSource.h"
#include <random>
#include <vector>

namespace Vehicle
{
    // Lane-based traffic around a fixed ego at the origin. Each lane is a conveyor: its cars share the
    // lane's speed and stay evenly spaced, so no car overtakes within a lane (there's no lane-change
    // model) and lanes never pile up. A car that runs off either end recycles to the far end WITH A NEW
    // id, so the binding sees a genuine despawn + spawn (drives the pool's slot get/return). The ego's
    // own lane (차선3) carries only pacing cars at the ego's speed, so nothing passes through the ego.
    class SyntheticSource : public IDataSource
    {
    public:
        explicit SyntheticSource(unsigned seed = 1337u);

        void             Update(float dt) override;
        const FrameData& GetCurrentFrame() const override { return frame; }
        const char*      GetName() const override { return "Synthetic"; }
        float            GetSensorHz() const override { return sensorHz; }

        // Stress knob: multiplies every lane's agent count and respawns the traffic. Exists to push
        // the renderer past the point where instancing can be dismissed as noise -- x1 is the demo
        // (16 tracks), x50 is ~800 tracks / ~7.5k scene objects. Cars beyond the first per lane slot
        // get a lateral spread inside the lane so a x50 lane reads as a swarm rather than one line
        // of coincident cars; x1 is left bit-identical to the pre-stress behaviour so it stays a
        // valid measurement baseline.
        void SetDensityMultiplier(int m);
        int  GetDensityMultiplier() const { return density; }
        int  GetTrackCount() const { return static_cast<int>(agents.size()); }

        // Geometry shared with the scene (ego placement, chase camera, lane markings).
        static constexpr float kEgoSpeed  = 14.0f;   // ~50 km/h
        static constexpr float kEgoLaneX  = 2.0f;    // ego sits in 차선3, at this X
        static constexpr float kEgoLen    = 4.4f;    // ego model target length / fallback box length
        static constexpr float kLaneWidth = 3.5f;
        // 차선1..4 centres. 1,2 are oncoming; 3 is the ego's lane; 4 is same-direction.
        static constexpr float kLaneX[4]  = {
            kEgoLaneX - 2.0f * kLaneWidth,
            kEgoLaneX - 1.0f * kLaneWidth,
            kEgoLaneX,
            kEgoLaneX + 1.0f * kLaneWidth,
        };

    private:
        struct Agent
        {
            float       relX  = 0.f;
            float       relZ  = 0.f;
            float       speed = 0.f;   // absolute m/s along +Z (negative = oncoming)
            ObjectClass cls   = ObjectClass::Car;
            float       bboxW = 1.9f;
            float       bboxL = 4.3f;
            int         id    = 0;
            int         lane  = 0;     // row in the lane table (SyntheticSource.cpp)
        };

        void BuildFrame();   // rebuild frame.ego + frame.objects from the current agents
        // (Re)creates the whole agent set at the current density, reseeding the rng first so the
        // same multiplier always produces the same traffic -- a measurement run has to be repeatable.
        void SpawnAgents();

        std::vector<Agent> agents;
        FrameData          frame;
        std::mt19937       rng;

        float egoSpeed = kEgoSpeed;
        float zBack    = -25.0f;   // recycle boundary behind the ego (behind the camera)
        float zFront   =  78.0f;   // recycle boundary ahead of the ego (past the visible road)
        float sensorHz = 30.0f;    // nominal sensor rate (drives the latency readout)
        int   nextId   = 1;
        int   density  = 1;        // see SetDensityMultiplier
        unsigned seedValue = 0;    // kept so SpawnAgents() can reseed rng
    };
}
