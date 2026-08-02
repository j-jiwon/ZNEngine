#pragma once
#include "FrameData.h"

// Per-id frame interpolation, shared by FrameInterpolator (sensor->render resample) and
// LogPlaybackSource (playhead between recorded frames). Presence follows the newer frame b;
// matched ids lerp, ids only in b (freshly spawned) take b as-is.
namespace Vehicle
{
    inline float Lerp1(float a, float b, float t) { return a + (b - a) * t; }

    inline const TrackedObject* FindById(const FrameData& f, int id)
    {
        for (const auto& o : f.objects)
            if (o.id == id) return &o;
        return nullptr;
    }

    inline EgoState LerpEgo(const EgoState& a, const EgoState& b, float t)
    {
        return EgoState{ Lerp1(a.speed,    b.speed,    t),
                         Lerp1(a.steering, b.steering, t),
                         Lerp1(a.yawRate,  b.yawRate,  t) };
    }

    // Blend a->b at t in [0,1]. timestamp is lerped too; callers that render at a different clock
    // (interpolator render time, log playhead) can overwrite out.timestamp afterwards.
    inline FrameData LerpFrame(const FrameData& a, const FrameData& b, float t)
    {
        FrameData out;
        out.timestamp = Lerp1(a.timestamp, b.timestamp, t);
        out.ego       = LerpEgo(a.ego, b.ego, t);
        out.objects.reserve(b.objects.size());
        for (const TrackedObject& ob : b.objects)
        {
            TrackedObject o = ob;
            if (const TrackedObject* oa = FindById(a, ob.id))
            {
                o.relX     = Lerp1(oa->relX,     ob.relX,     t);
                o.relZ     = Lerp1(oa->relZ,     ob.relZ,     t);
                o.relSpeed = Lerp1(oa->relSpeed, ob.relSpeed, t);
                o.bboxW    = Lerp1(oa->bboxW,    ob.bboxW,    t);
                o.bboxL    = Lerp1(oa->bboxL,    ob.bboxL,    t);
            }
            out.objects.push_back(o);
        }
        return out;
    }
}
