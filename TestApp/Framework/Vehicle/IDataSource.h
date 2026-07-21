#pragma once
#include "FrameData.h"

// Abstract source of sensor frames. The whole point of this layer: the audio visualiser
// (mirror-ball) and the sensor visualiser are two sources behind the SAME interface, so the
// scene binding never cares where a FrameData came from.
//
//   IDataSource
//     |- SyntheticSource     procedural traffic (this slice)
//     |- LogPlaybackSource   JSON/CSV replay      (later: scrub/rewind/speed)
//     |- AudioSource         FFT -> parameters    (later: absorbs the mirror-ball visualiser)
namespace Vehicle
{
    class IDataSource
    {
    public:
        virtual ~IDataSource() = default;

        // Advance the source by dt seconds and refresh the current frame.
        virtual void Update(float dt) = 0;

        // The most recent frame (valid until the next Update).
        virtual const FrameData& GetCurrentFrame() const = 0;

        // Short display name for the DataSource UI panel.
        virtual const char* GetName() const = 0;

        // ---- Shared playback controls (default-backed; LogPlayback adds SetPlayhead later) ----
        void  SetPaused(bool p) { paused = p; }
        bool  IsPaused() const  { return paused; }
        void  SetSpeed(float s) { speed = s; }   // 1.0 = realtime, 2.0 = 2x, 0.5 = half...
        float GetSpeed() const  { return speed; }

    protected:
        bool  paused = false;
        float speed  = 1.0f;
    };
}
