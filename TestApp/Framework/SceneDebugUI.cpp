#include "SceneDebugUI.h"
#include "SceneManager.h"
#include <ZNFramework.h>
#include <imgui.h>
#include <Windows.h>
#include <string>

using namespace ZNFramework;

SceneDebugUI::SceneDebugUI()
    : lastTime(Clock::now())
{
}

void SceneDebugUI::Render(ZNScene* scene)
{
    // Always tick timing so fps doesn't spike on re-show.
    auto now = Clock::now();
    float dt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;

    fpsAccum  += dt;
    fpsFrames += 1;
    if (fpsAccum >= 0.5f)
    {
        fpsDisplay = fpsFrames / fpsAccum;
        fpsAccum   = 0.0f;
        fpsFrames  = 0;

        FILETIME idle, kernel, user;
        if (GetSystemTimes(&idle, &kernel, &user))
        {
            auto toU64 = [](const FILETIME& ft) -> unsigned long long {
                return (unsigned long long(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
            };
            unsigned long long idleU   = toU64(idle);
            unsigned long long kernelU = toU64(kernel);
            unsigned long long userU   = toU64(user);
            unsigned long long idleDelta  = idleU   - prevIdle;
            unsigned long long totalDelta = (kernelU - prevKernel) + (userU - prevUser);
            if (totalDelta > 0)
                cpuUsage = (1.0f - float(idleDelta) / totalDelta) * 100.0f;
            prevIdle   = idleU;
            prevKernel = kernelU;
            prevUser   = userU;
        }
    }

    if (!visible) return;

    // --- Stats ---
    const float cpuMs        = fpsDisplay > 0.0f ? 1000.0f / fpsDisplay : 0.0f;
    const float gpuMs        = GraphicsContext::GetInstance().GetCommandQueue()
                             ? GraphicsContext::GetInstance().GetCommandQueue()->GetGpuFrameTimeMs()
                             : 0.0f;
    const float gpuMemUsedMB = GraphicsContext::GetInstance().GetDevice()
                             ? GraphicsContext::GetInstance().GetDevice()->GetGpuMemoryUsageMB()
                             : 0.0f;
    const float gpuMemBudgMB = GraphicsContext::GetInstance().GetDevice()
                             ? GraphicsContext::GetInstance().GetDevice()->GetGpuMemoryBudgetMB()
                             : 0.0f;

    int totalObjs = 0, visibleObjs = 0;
    if (scene)
    {
        for (auto* o : scene->GetGameObjects())
        {
            ++totalObjs;
            if (o->IsVisible()) ++visibleObjs;
        }
    }

    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(220.0f, 0.0f), ImGuiCond_Always);
    ImGui::Begin("Stats", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("FPS        %.1f",  fpsDisplay);
    ImGui::Text("CPU        %.2f ms", cpuMs);
    ImGui::Text("CPU Use    %.1f%%", cpuUsage);
    ImGui::Text("GPU        %.2f ms", gpuMs);
    ImGui::Separator();
    ImGui::Text("Draw Calls %d",    ZNGameObject::GetLastFrameDrawCalls());
    ImGui::Text("Objects    %d / %d", visibleObjs, totalObjs);
    ImGui::Separator();
    ImGui::Text("GPU Mem    %.0f / %.0f MB", gpuMemUsedMB, gpuMemBudgMB);
    ImGui::Text("Triangles  %d",    ZNGameObject::GetLastFrameTriangles());
    ImGui::Text("Vertices   %d",    ZNGameObject::GetLastFrameVertices());

    ImGui::End();

    // --- Outliner ---
    ImGui::SetNextWindowSize(ImVec2(220.0f, 400.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Outliner");

    if (ImGui::TreeNodeEx("GameObjects", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (scene)
        {
            for (auto* obj : scene->GetGameObjects())
            {
                const auto& tag = obj->GetTag();
                if (tag == "Debug" || tag == "Room") continue;
                bool isSelected = (selection.type == SelectionType::GameObject && selection.ptr == obj);
                if (ImGui::Selectable(obj->GetName().c_str(), isSelected))
                {
                    selection.type = SelectionType::GameObject;
                    selection.ptr  = obj;
                }
            }
        }
        ImGui::TreePop();
    }

    // Scene-specific extras (e.g. "Cameras" section from TestGameScene)
    if (onOutlinerExtras)
        onOutlinerExtras();

    if (ImGui::TreeNodeEx("Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (scene)
        {
            const auto& spots = scene->GetSpotLights();
            for (int i = 0; i < (int)spots.size(); ++i)
            {
                std::string label = "SpotLight " + std::to_string(i);
                bool isSelected = (selection.type == SelectionType::SpotLight && selection.ptr == spots[i]);
                if (ImGui::Selectable(label.c_str(), isSelected))
                {
                    selection.type = SelectionType::SpotLight;
                    selection.ptr  = spots[i];
                }
            }
            ZNDirectionalLight* dl = scene->GetDirectionalLight();
            if (dl)
            {
                bool isSelected = (selection.type == SelectionType::DirectionalLight && selection.ptr == dl);
                if (ImGui::Selectable("DirectionalLight", isSelected))
                {
                    selection.type = SelectionType::DirectionalLight;
                    selection.ptr  = dl;
                }
            }
        }
        ImGui::TreePop();
    }

    ImGui::End();

    // --- Inspector ---
    ImGui::SetNextWindowSize(ImVec2(280.0f, 400.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector");

    switch (selection.type)
    {
    case SelectionType::GameObject:
    {
        ZNGameObject* obj = static_cast<ZNGameObject*>(selection.ptr);
        ImGui::Text("GameObject: %s", obj->GetName().c_str());
        ImGui::Separator();
        ImGui::Text("Transform");
        Transform& t = obj->GetTransform();
        float pos[3] = { t.position.x, t.position.y, t.position.z };
        if (ImGui::DragFloat3("Position", pos, 0.01f))
            t.position = ZNVector3(pos[0], pos[1], pos[2]);
        float rot[3] = { t.rotation.x, t.rotation.y, t.rotation.z };
        if (ImGui::DragFloat3("Rotation", rot, 0.5f))
            t.rotation = ZNVector3(rot[0], rot[1], rot[2]);
        float sc[3] = { t.scale.x, t.scale.y, t.scale.z };
        if (ImGui::DragFloat3("Scale", sc, 0.001f, 0.001f, 100.0f))
            t.scale = ZNVector3(sc[0], sc[1], sc[2]);
        ZNMaterial* mat = obj->GetMaterial();
        if (mat)
        {
            ImGui::Separator();
            ImGui::Text("Material");
            MaterialParams p = mat->GetParams();
            bool changed = false;
            float col[4] = { p.albedoColor.x, p.albedoColor.y, p.albedoColor.z, p.albedoColor.w };
            if (ImGui::ColorEdit4("Albedo", col))
            {
                p.albedoColor = ZNVector4(col[0], col[1], col[2], col[3]);
                changed = true;
            }
            if (ImGui::SliderFloat("Metallic",  &p.metallic,  0.0f, 1.0f)) changed = true;
            if (ImGui::SliderFloat("Roughness", &p.roughness, 0.0f, 1.0f)) changed = true;
            if (changed) mat->SetParams(p);
        }
        break;
    }
    case SelectionType::SpotLight:
    {
        ZNSpotLight* light = static_cast<ZNSpotLight*>(selection.ptr);
        ImGui::Text("SpotLight");
        ImGui::Separator();
        ZNVector3 col = light->GetColor();
        float color[3] = { col.x, col.y, col.z };
        if (ImGui::ColorEdit3("Color", color))
            light->SetColor(ZNVector3(color[0], color[1], color[2]));
        float intensity = light->GetIntensity();
        if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f))
            light->SetIntensity(intensity);
        ZNVector3 pos = light->GetPosition();
        float posArr[3] = { pos.x, pos.y, pos.z };
        if (ImGui::DragFloat3("Position", posArr, 0.05f))
            light->SetPosition(ZNVector3(posArr[0], posArr[1], posArr[2]));
        float innerAngle = light->GetInnerCutoffAngle();
        float outerAngle = light->GetOuterCutoffAngle();
        bool cutoffChanged = false;
        if (ImGui::SliderFloat("Inner Angle", &innerAngle, 0.0f, 89.0f)) cutoffChanged = true;
        if (ImGui::SliderFloat("Outer Angle", &outerAngle, 0.0f, 89.0f)) cutoffChanged = true;
        if (cutoffChanged) light->SetCutoffAngle(innerAngle, outerAngle);
        break;
    }
    case SelectionType::DirectionalLight:
    {
        ZNDirectionalLight* dl = static_cast<ZNDirectionalLight*>(selection.ptr);
        ImGui::Text("DirectionalLight");
        ImGui::Separator();
        ZNVector3 col = dl->GetColor();
        float color[3] = { col.x, col.y, col.z };
        if (ImGui::ColorEdit3("Color", color))
            dl->SetColor(ZNVector3(color[0], color[1], color[2]));
        float intensity = dl->GetIntensity();
        if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 20.0f))
            dl->SetIntensity(intensity);
        ZNVector3 dir = dl->GetDirection();
        float dirArr[3] = { dir.x, dir.y, dir.z };
        if (ImGui::SliderFloat3("Direction", dirArr, -1.0f, 1.0f))
            dl->SetDirection(ZNVector3(dirArr[0], dirArr[1], dirArr[2]).Normalize());
        break;
    }
    case SelectionType::Custom:
        if (onInspectorExtras)
            onInspectorExtras(selection.ptr);
        break;
    default:
        ImGui::TextDisabled("Nothing selected.");
        break;
    }

    ImGui::End();

    // --- Scenes ---
    ImGui::SetNextWindowSize(ImVec2(180.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scenes");
    const auto& entries = SceneManager::Get().GetEntries();
    int current = SceneManager::Get().GetCurrentIndex();
    for (int i = 0; i < (int)entries.size(); i++)
    {
        bool isActive = (i == current);
        if (isActive)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.15f, 1.0f));
        if (ImGui::Button(entries[i].name.c_str(), ImVec2(-1, 0)))
            SceneManager::Get().SwitchTo(i);
        if (isActive)
            ImGui::PopStyleColor();
    }
    ImGui::End();
}

void SceneDebugUI::RenderToggle()
{
    if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
        visible = !visible;

    auto& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 58.0f, 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(48.0f, 30.0f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##ui_toggle", nullptr,
        ImGuiWindowFlags_NoTitleBar      | ImGuiWindowFlags_NoResize    |
        ImGuiWindowFlags_NoScrollbar     | ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImVec4 btnColor = visible
        ? ImVec4(0.15f, 0.50f, 0.15f, 1.0f)
        : ImVec4(0.45f, 0.15f, 0.15f, 1.0f);
    ImVec4 hoverColor(btnColor.x + 0.1f, btnColor.y + 0.1f, btnColor.z + 0.1f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,        btnColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  hoverColor);
    if (ImGui::Button(visible ? " UI " : " -- ", ImVec2(40.0f, 22.0f)))
        visible = !visible;
    ImGui::PopStyleColor(3);

    ImGui::End();
    ImGui::PopStyleVar(2);
}
