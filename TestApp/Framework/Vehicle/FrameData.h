#pragma once
#include <vector>

// Sensor-frame schema. A data source emits one FrameData per tick: a relative-coordinate snapshot
// of what's around the ego now. Ego is fixed at the origin; all coords are ego-relative.
namespace Vehicle
{
    enum class ObjectClass { Car, Pedestrian, Cyclist };

    inline const char* ToString(ObjectClass c)
    {
        switch (c)
        {
            case ObjectClass::Car:        return "car";
            case ObjectClass::Pedestrian: return "ped";
            case ObjectClass::Cyclist:    return "cyclist";
        }
        return "?";
    }

    struct EgoState
    {
        float speed    = 0.f;  // m/s (forward)
        float steering = 0.f;  // rad (steering wheel angle, unused in the vertical slice)
        float yawRate  = 0.f;  // rad/s
    };

    // One tracked neighbour. Coordinates are ego-relative: +X = right, +Z = forward.
    struct TrackedObject
    {
        int         id       = 0;                  // stable across frames (same id = same object)
        ObjectClass cls      = ObjectClass::Car;
        float       relX     = 0.f;                // metres right of ego
        float       relZ     = 0.f;                // metres ahead of ego
        float       relSpeed = 0.f;                // closing speed along Z (m/s); + = pulling away
        float       bboxW    = 1.8f;               // width  (X extent, metres)
        float       bboxL    = 4.2f;               // length (Z extent, metres)
    };

    struct FrameData
    {
        float                      timestamp = 0.f;   // seconds since the source started
        EgoState                   ego;
        std::vector<TrackedObject> objects;
        // LanePolyline lanes; // omitted in the vertical slice (scene draws scrolling lane dashes)
    };
}
