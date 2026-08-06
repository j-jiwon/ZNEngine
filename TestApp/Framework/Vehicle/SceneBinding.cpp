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

        void ApplyTransform(ZNGameObject* obj, const TrackedObject& o, VehicleScene& scene)
        {
            Transform& t = obj->GetTransform();
            // Face travel direction: left-hand lanes head the other way.
            const float faceYaw = (o.relX < -1.0f) ? 180.f : 0.f;

            if (o.cls == ObjectClass::Car && scene.HasCarModel())
            {
                const float s = scene.GetCarFitScale();
                t.scale    = ZNVector3(s, s, s);
                t.position = ZNVector3(o.relX, scene.GetCarGroundLift(), o.relZ);
                t.rotation = ZNVector3(0.f, faceYaw + scene.GetCarForwardYaw(), 0.f);
                return;
            }

            if (o.cls == ObjectClass::Pedestrian || o.cls == ObjectClass::Cyclist)
            {
                // Body/head/frame are fixed-proportion children (SpawnHumanoidInstance) so they read
                // as a person regardless of the sensor's detected bbox — root only positions/faces it.
                t.position = ZNVector3(o.relX, 0.0f, o.relZ);
                t.rotation = ZNVector3(0.f, faceYaw, 0.f);
                return;
            }

            const float h = ClassHeight(o.cls);
            t.scale    = ZNVector3(o.bboxW, h, o.bboxL);
            t.position = ZNVector3(o.relX, h * 0.5f, o.relZ);   // sit on the ground plane
            t.rotation = ZNVector3(0.f, faceYaw, 0.f);
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

            if (!obj)   // new id -> new object (pool recycles the slot, not the object)
            {
                ZNObjectHandle h;
                const std::string name = std::string(ToString(o.cls)) + " #" + std::to_string(o.id);
                if (o.cls == ObjectClass::Car && scene.HasCarModel())
                {
                    h = scene.SpawnCarTrack(name);
                }
                else if (o.cls == ObjectClass::Pedestrian || o.cls == ObjectClass::Cyclist)
                {
                    h = scene.SpawnHumanoidInstance(o.cls, name, "Track");
                }
                else
                {
                    obj = new ZNGameObject();
                    obj->SetMesh(scene.GetClassMesh(o.cls));
                    obj->SetMaterial(scene.GetClassMaterial(o.cls));
                    obj->SetTag("Track");
                    obj->SetCastShadow(true);
                    h = scene.AddGameObject(obj);
                }
                trackToHandle[o.id] = h;
                obj = scene.Resolve(h);
            }

            obj->SetName(std::string(ToString(o.cls)) + " #" + std::to_string(o.id));
            ApplyTransform(obj, o, scene);
        }

        // absent this frame = left the sensor range -> destroy (frees object + releases the slot).
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
