#include "SceneBinding.h"
#include "../VehicleScene.h"
#include <ZNFramework.h>
#include <string>
#include <unordered_set>

using namespace ZNFramework;

namespace Vehicle
{
    namespace
    {
        float ClassHeight(ObjectClass c)
        {
            switch (c)
            {
                case ObjectClass::Car:        return 1.4f;
                case ObjectClass::Cyclist:    return 1.6f;
                case ObjectClass::Pedestrian: return 1.7f;
            }
            return 1.0f;
        }

        void ApplyTransform(ZNGameObject* obj, const TrackedObject& o)
        {
            const float h = ClassHeight(o.cls);
            Transform& t = obj->GetTransform();
            t.scale    = ZNVector3(o.bboxW, h, o.bboxL);
            t.position = ZNVector3(o.relX, h * 0.5f, o.relZ);   // sit on the ground plane
            // Face travel direction: left-hand lanes head the other way.
            t.rotation = ZNVector3(0.f, (o.relX < -1.0f) ? 180.f : 0.f, 0.f);
        }
    }

    void SceneBinding::Apply(const FrameData& frame, VehicleScene& scene)
    {
        std::unordered_set<int> present;
        present.reserve(frame.objects.size());

        for (const auto& o : frame.objects)
        {
            present.insert(o.id);

            auto it = trackToHandle.find(o.id);
            ZNGameObject* obj = (it != trackToHandle.end()) ? scene.Resolve(it->second) : nullptr;

            if (!obj)   // new track -> pull a fresh object from the pool
            {
                obj = new ZNGameObject();
                obj->SetMesh(scene.GetClassMesh(o.cls));
                obj->SetMaterial(scene.GetClassMaterial(o.cls));
                obj->SetTag("Track");
                obj->SetCastShadow(true);
                ZNObjectHandle h = scene.AddGameObject(obj);
                trackToHandle[o.id] = h;
            }

            obj->SetName(std::string(ToString(o.cls)) + " #" + std::to_string(o.id));
            ApplyTransform(obj, o);
        }

        // Sweep: any track absent from this frame has left the sensor range -> return to the pool.
        for (auto it = trackToHandle.begin(); it != trackToHandle.end(); )
        {
            if (present.find(it->first) == present.end())
            {
                scene.Destroy(it->second);
                it = trackToHandle.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}
