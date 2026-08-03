#include "LogFormat.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <string>

using nlohmann::json;

namespace Vehicle
{
    namespace
    {
        ObjectClass ParseClass(const std::string& s)
        {
            if (s == "ped")     return ObjectClass::Pedestrian;
            if (s == "cyclist") return ObjectClass::Cyclist;
            return ObjectClass::Car;
        }
    }

    bool WriteLog(const std::filesystem::path& path, const LogData& log)
    {
        json root;
        root["sensorHz"] = log.sensorHz;

        json frames = json::array();
        for (const FrameData& f : log.frames)
        {
            json jf;
            jf["t"]   = f.timestamp;
            jf["ego"] = { {"speed", f.ego.speed}, {"steering", f.ego.steering}, {"yawRate", f.ego.yawRate} };

            json objs = json::array();
            for (const TrackedObject& o : f.objects)
            {
                objs.push_back({
                    {"id", o.id}, {"cls", ToString(o.cls)},
                    {"relX", o.relX}, {"relZ", o.relZ}, {"relSpeed", o.relSpeed},
                    {"bboxW", o.bboxW}, {"bboxL", o.bboxL} });
            }
            jf["objects"] = std::move(objs);
            frames.push_back(std::move(jf));
        }
        root["frames"] = std::move(frames);

        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec); // best-effort
        std::ofstream os(path, std::ios::trunc);
        if (!os) return false;
        os << root.dump(2);
        return static_cast<bool>(os);
    }

    bool ReadLog(const std::filesystem::path& path, LogData& out)
    {
        std::ifstream is(path);
        if (!is) return false;

        json root;
        try { is >> root; }
        catch (const json::exception&) { return false; }

        out.sensorHz = root.value("sensorHz", 30.0f);
        out.frames.clear();

        if (root.contains("frames") && root["frames"].is_array())
        {
            for (const auto& jf : root["frames"])
            {
                FrameData f;
                f.timestamp = jf.value("t", 0.0f);
                if (jf.contains("ego"))
                {
                    const auto& e = jf["ego"];
                    f.ego.speed    = e.value("speed",    0.0f);
                    f.ego.steering = e.value("steering", 0.0f);
                    f.ego.yawRate  = e.value("yawRate",  0.0f);
                }
                if (jf.contains("objects") && jf["objects"].is_array())
                {
                    for (const auto& jo : jf["objects"])
                    {
                        TrackedObject o;
                        o.id       = jo.value("id", 0);
                        o.cls      = ParseClass(jo.value("cls", std::string("car")));
                        o.relX     = jo.value("relX",     0.0f);
                        o.relZ     = jo.value("relZ",     0.0f);
                        o.relSpeed = jo.value("relSpeed", 0.0f);
                        o.bboxW    = jo.value("bboxW",    1.8f);
                        o.bboxL    = jo.value("bboxL",    4.2f);
                        f.objects.push_back(o);
                    }
                }
                out.frames.push_back(std::move(f));
            }
        }

        std::sort(out.frames.begin(), out.frames.end(),
                  [](const FrameData& a, const FrameData& b) { return a.timestamp < b.timestamp; });
        return !out.frames.empty();
    }
}
