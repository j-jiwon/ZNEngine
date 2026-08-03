#include "LogPlaybackSource.h"
#include "FrameLerp.h"
#include <algorithm>
#include <cmath>

namespace Vehicle
{
    bool LogPlaybackSource::Load(const std::filesystem::path& path)
    {
        if (!ReadLog(path, log))
            return false;
        duration = log.frames.empty() ? 0.0f : log.frames.back().timestamp;
        playhead = 0.0f;
        Rebuild();
        return true;
    }

    void LogPlaybackSource::Update(float dt)
    {
        if (log.frames.empty()) return;

        playhead += dt;   // speed/pause are applied by the FrameInterpolator (the clock driver)
        if (loop && duration > 0.0f)
        {
            playhead = std::fmod(playhead, duration);
            if (playhead < 0.0f) playhead += duration;
        }
        else
        {
            playhead = std::clamp(playhead, 0.0f, duration);
        }
        Rebuild();
    }

    void LogPlaybackSource::SetPlayhead(float t)
    {
        if (duration > 0.0f) t = std::clamp(t, 0.0f, duration);
        playhead = t;
        Rebuild();
    }

    void LogPlaybackSource::Rebuild()
    {
        const auto& fs = log.frames;
        if (fs.empty()) { current = FrameData{}; return; }
        if (playhead <= fs.front().timestamp) { current = fs.front(); return; }
        if (playhead >= fs.back().timestamp)  { current = fs.back();  return; }

        size_t i = 0;
        while (i + 1 < fs.size() && fs[i + 1].timestamp < playhead)
            ++i;
        const FrameData& a = fs[i];
        const FrameData& b = fs[i + 1];
        const float span = b.timestamp - a.timestamp;
        const float t    = (span > 1e-6f) ? (playhead - a.timestamp) / span : 0.0f;

        current = LerpFrame(a, b, t);
        current.timestamp = playhead;
    }
}
