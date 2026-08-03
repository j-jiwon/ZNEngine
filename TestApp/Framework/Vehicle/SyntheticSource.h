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

        std::vector<Agent> agents;
        FrameData          frame;
        std::mt19937       rng;

        float egoSpeed = kEgoSpeed;
        float zBack    = -25.0f;   // recycle boundary behind the ego (behind the camera)
        float zFront   =  78.0f;   // recycle boundary ahead of the ego (past the visible road)
        float sensorHz = 30.0f;    // nominal sensor rate (drives the latency readout)
        int   nextId   = 1;
    };
}
