#include "FrameInterpolator.h"
#include "IDataSource.h"
#include <algorithm>

namespace Vehicle
{
    namespace
    {
        constexpr int   kMaxHistory = 4;    // enough to bracket ~1 period back with slack
        constexpr int   kTickGuard  = 8;    // cap sensor ticks per render frame (big dt / speed jump)
        constexpr float kMaxLead    = 2.0f; // extrapolate at most this many periods ahead (overshoot guard)

        inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

        const TrackedObject* Find(const FrameData& f, int id)
        {
            for (const auto& o : f.objects)
                if (o.id == id) return &o;
            return nullptr;
        }

        EgoState LerpEgo(const EgoState& a, const EgoState& b, float t)
        {
            return EgoState{ Lerp(a.speed,    b.speed,    t),
                             Lerp(a.steering, b.steering, t),
                             Lerp(a.yawRate,  b.yawRate,  t) };
        }
    }

    void FrameInterpolator::Init(IDataSource& src, float hz)
    {
        source = &src;
        SetSensorHz(hz);
        history.clear();
        simClock       = 0.f;
        lastSampleSim  = 0.f;
        nextSampleTime = 0.f;
    }

    void FrameInterpolator::SetSensorHz(float hz)
    {
        sensorHz     = (hz > 1.0f) ? hz : 1.0f;
        sensorPeriod = 1.0f / sensorHz;
    }

    float FrameInterpolator::NextInterval()
    {
        if (jitterFrac <= 0.0f)
            return sensorPeriod;
        // +/- jitterFrac of a period on the gap between arrivals; keep it safely positive.
        std::uniform_real_distribution<float> j(-jitterFrac, jitterFrac);
        return sensorPeriod * std::max(0.1f, 1.0f + j(rng));
    }

    void FrameInterpolator::Advance(float toSimTime, bool record)
    {
        const float dt = std::max(0.0f, toSimTime - lastSampleSim);
        source->Update(dt);                      // move the world forward to this sample time
        lastSampleSim = toSimTime;
        if (!record)                             // dropout: the world moved but no reading arrived
            return;

        FrameData snap = source->GetCurrentFrame();
        snap.timestamp = toSimTime;              // stamp with the sensor clock (sim seconds)
        history.push_back(std::move(snap));
        while (history.size() > kMaxHistory) history.pop_front();
    }

    void FrameInterpolator::Update(float dtReal)
    {
        if (!source) return;
        if (source->IsPaused())
            return;                              // freeze the clock; Sample() keeps the last pose

        simClock += dtReal * source->GetSpeed();

        // Catch the sensor clock up to sim time (guard bounds the backlog after a long stall).
        int ticks = 0;
        while (nextSampleTime <= simClock && ticks++ < kTickGuard)
        {
            std::uniform_real_distribution<float> u(0.0f, 1.0f);
            // Never drop the first sample, or the scene has nothing to show.
            const bool drop = !history.empty() && dropoutProb > 0.0f && u(rng) < dropoutProb;
            Advance(nextSampleTime, !drop);
            nextSampleTime += NextInterval();
        }
    }

    const FrameData& FrameInterpolator::Sample()
    {
        if (history.empty())
        {
            output = FrameData{};
            return output;
        }

        // One snapshot (or Off): nothing to blend between -> newest sensor truth, as-is.
        if (mode == Mode::Off || history.size() == 1)
        {
            output = history.back();
            return output;
        }

        if (mode == Mode::Extrapolate)
        {
            BuildExtrapolated(simClock);         // predict forward up to "now"
            return output;
        }

        // Interpolate: render interpDelay periods in the past so two snapshots always bracket us.
        BuildInterpolated(simClock - interpDelayPeriods * sensorPeriod);
        return output;
    }

    void FrameInterpolator::BuildInterpolated(float renderTime)
    {
        const FrameData& front = history.front();
        const FrameData& back  = history.back();

        // Outside the buffered window -> clamp to an endpoint (never extrapolate here).
        if (renderTime <= front.timestamp) { output = front; return; }
        if (renderTime >= back.timestamp)  { output = back;  return; }

        // Find the bracketing pair a <= renderTime <= b.
        size_t i = 0;
        while (i + 1 < history.size() && history[i + 1].timestamp < renderTime)
            ++i;
        const FrameData& a = history[i];
        const FrameData& b = history[i + 1];

        const float span = b.timestamp - a.timestamp;
        const float t    = (span > 1e-6f) ? std::clamp((renderTime - a.timestamp) / span, 0.f, 1.f) : 0.f;

        // Presence follows the newer frame b; matched ids lerp, freshly-spawned ids take b as-is.
        output.timestamp = renderTime;
        output.ego       = LerpEgo(a.ego, b.ego, t);
        output.objects.clear();
        output.objects.reserve(b.objects.size());
        for (const TrackedObject& ob : b.objects)
        {
            TrackedObject o = ob;
            if (const TrackedObject* oa = Find(a, ob.id))
            {
                o.relX     = Lerp(oa->relX,     ob.relX,     t);
                o.relZ     = Lerp(oa->relZ,     ob.relZ,     t);
                o.relSpeed = Lerp(oa->relSpeed, ob.relSpeed, t);
                o.bboxW    = Lerp(oa->bboxW,    ob.bboxW,    t);
                o.bboxL    = Lerp(oa->bboxL,    ob.bboxL,    t);
            }
            output.objects.push_back(o);
        }
    }

    void FrameInterpolator::BuildExtrapolated(float renderTime)
    {
        const FrameData& b = history.back();
        const float lead = std::clamp(renderTime - b.timestamp, 0.f, kMaxLead * sensorPeriod);

        output.timestamp = renderTime;
        output.ego       = b.ego;
        output.objects.clear();
        output.objects.reserve(b.objects.size());
        for (const TrackedObject& ob : b.objects)
        {
            TrackedObject o = ob;
            o.relZ += ob.relSpeed * lead;        // relSpeed is closing speed along Z; no lateral term in schema
            output.objects.push_back(o);
        }
    }

    float FrameInterpolator::EffectiveLatencyMs() const
    {
        // Only Interpolate adds latency (renders in the past); Off/Extrapolate show ~newest truth.
        if (mode == Mode::Interpolate)
            return interpDelayPeriods * sensorPeriod * 1000.0f;
        return 0.0f;
    }

    float FrameInterpolator::StaleMs() const
    {
        if (history.empty()) return 0.0f;
        return std::max(0.0f, (simClock - history.back().timestamp) * 1000.0f);
    }
}
