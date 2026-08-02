#pragma once
#include "FrameData.h"
#include <vector>
#include <filesystem>

// JSON (de)serialization of a recorded scenario. Schema mirrors FrameData 1:1 (automotive-specific,
// object-list shaped): { sensorHz, frames:[ { t, ego{...}, objects:[{id,cls,relX,relZ,...}] } ] }.
// Uses nlohmann/json (ZN/ZNFramework/ThirdParty) so hand-edited logs parse robustly too.
namespace Vehicle
{
    struct LogData
    {
        float                      sensorHz = 30.0f;
        std::vector<FrameData>     frames;   // ascending timestamp
    };

    bool WriteLog(const std::filesystem::path& path, const LogData& log);
    bool ReadLog(const std::filesystem::path& path, LogData& out);   // sorts frames by timestamp
}
