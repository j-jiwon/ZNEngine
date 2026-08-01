#pragma once
#include "FrameData.h"
#include <deque>
#include <random>

namespace Vehicle
{
    class IDataSource;

    // Resamples a discrete sensor source (ticking at sensorHz) up to the render frame rate, so the
    // scene stays smooth instead of jumping one sensor step at a time. Buffers recent snapshots and
    // each frame rebuilds an in-between FrameData:
    //   Interpolate : lerp between the two snapshots bracketing (now - interpDelay). Smooth, +latency.
    //   Extrapolate : predict forward from the newest snapshot along relSpeed. ~zero latency, overshoots.
    //   Off         : newest snapshot as-is. Visible sensor-rate judder.
    // Speed/pause are read from the source and applied here on the clock, so the source stays a plain
    // "advance by dt" generator and the fixed-cadence tick is never scaled twice.
    class FrameInterpolator
    {
    public:
        enum class Mode { Off, Interpolate, Extrapolate };

        // Bind the source to resample. sensorHz is the source's nominal (starting) tick rate.
        void Init(IDataSource& source, float sensorHz);

        // Advance real time by dt; ticks the source at the sensor cadence, buffering snapshots.
        void Update(float dtReal);

        // The resampled frame at the current render time (per current mode). Valid until next Update/Sample.
        const FrameData& Sample();

        // --- controls / readouts (DataSource panel) ---
        void  SetMode(Mode m)                { mode = m; }
        Mode  GetMode() const                { return mode; }
        void  SetInterpDelayPeriods(float p) { interpDelayPeriods = p; }
        float GetInterpDelayPeriods() const  { return interpDelayPeriods; }
        void  SetSensorHz(float hz);         // re-derives the tick period (min 1 Hz)
        float GetSensorHz() const            { return sensorHz; }
        void  SetDropoutProb(float p)        { dropoutProb = p; }  // 0..1 chance a sample is missed
        float GetDropoutProb() const         { return dropoutProb; }
        void  SetJitterFrac(float j)         { jitterFrac = j; }   // 0..1 of a period, +/- on arrival time
        float GetJitterFrac() const          { return jitterFrac; }
        int   BufferedFrames() const         { return static_cast<int>(history.size()); }
        // Latency the current mode adds, in ms (Interpolate only).
        float EffectiveLatencyMs() const;
        // Age of the newest snapshot in ms (spikes during dropouts).
        float StaleMs() const;

    private:
        void Advance(float toSimTime, bool record);   // move the source to a sample time, maybe record it
        void BuildInterpolated(float renderTime);
        void BuildExtrapolated(float renderTime);
        float NextInterval();                          // one period, perturbed by jitter

        IDataSource* source       = nullptr;
        float        sensorHz     = 30.0f;
        float        sensorPeriod = 1.0f / 30.0f;

        std::deque<FrameData> history;           // recent snapshots, timestamps ascending (sim seconds)
        float simClock       = 0.f;              // simulation seconds elapsed (scaled by playback speed)
        float lastSampleSim  = 0.f;              // sim time the source was last advanced to
        float nextSampleTime = 0.f;              // sim time of the next scheduled sample

        Mode  mode               = Mode::Interpolate;
        float interpDelayPeriods = 1.0f;         // render this many sensor periods in the past

        // Sensor-imperfection knobs (default off).
        float dropoutProb = 0.0f;                // per-sample probability the reading is missed
        float jitterFrac  = 0.0f;                // arrival-time jitter as a fraction of a period
        std::mt19937 rng{ 20260802u };

        FrameData output;                        // rebuilt each Sample()
    };
}
