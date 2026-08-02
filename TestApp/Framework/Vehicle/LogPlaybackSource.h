#pragma once
#include "IDataSource.h"
#include "LogFormat.h"
#include <filesystem>

// Replays a recorded JSON scenario. The third IDataSource behind the same seam as SyntheticSource:
// the binding/scene/panel don't change. Adds seeking (SetPlayhead) — the deterministic re-examination
// that a procedural source can't give. Update() advances the playhead by dt (the FrameInterpolator
// drives it at the sensor cadence and applies speed/pause), so the composition matches SyntheticSource.
namespace Vehicle
{
    class LogPlaybackSource : public IDataSource
    {
    public:
        bool Load(const std::filesystem::path& path);

        void             Update(float dt) override;
        const FrameData& GetCurrentFrame() const override { return current; }
        const char*      GetName() const override { return "Log"; }
        float            GetSensorHz() const override { return log.sensorHz; }

        // --- timeline / seek (stage 5) ---
        void  SetPlayhead(float t);            // jump to a time; resample current
        float GetPlayhead() const { return playhead; }
        float GetDuration() const { return duration; }
        int   FrameCount() const  { return static_cast<int>(log.frames.size()); }
        void  SetLoop(bool l)     { loop = l; }
        bool  IsLoop() const      { return loop; }
        bool  IsLoaded() const    { return !log.frames.empty(); }

    private:
        void Rebuild();   // current = frame at playhead (lerp between bracketing recorded frames)

        LogData   log;
        float     playhead = 0.0f;
        float     duration = 0.0f;
        bool      loop     = true;
        FrameData current;
    };
}
