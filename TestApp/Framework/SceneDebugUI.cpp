#include "SceneDebugUI.h"
#include "SceneManager.h"
#include <ZNFramework.h>
#include <imgui.h>
#include <Windows.h>
#include <algorithm>
#include <cmath>

using namespace ZNFramework;

static constexpr float kPI = 3.14159265f;

SceneDebugUI::SceneDebugUI()
    : lastTime(Clock::now())
{
}

// ---------------------------------------------------------------------------
// Debug overlay helpers
// ---------------------------------------------------------------------------

void SceneDebugUI::EnsureDebugShaders()
{
    if (dbgSolidShader && dbgAlphaShader) return;

    if (!dbgSolidShader) {
        dbgSolidShader = Platform::CreateShader();
        dbgSolidShader->Load(GetResourcePath() / L"Shaders" / L"forward_unlit.hlsli");
    }
    if (!dbgAlphaShader) {
        dbgAlphaShader = Platform::CreateShader();
        dbgAlphaShader->Load(GetResourcePath() / L"Shaders" / L"forward_unlit.hlsli");
        dbgAlphaShader->EnableAlphaBlend();
        dbgAlphaShader->DisableDepthWrite();
    }
}

void SceneDebugUI::OnSceneChanged(ZNScene* scene)
{
    // Old entries become orphaned (not deleted — GPU may still reference their meshes).
    // Bounded memory: one set per scene switch, reclaimed on app exit.
    spotEntries.clear();
    camEntries.clear();
    showSpotIndicators = false;
    showCamIndicators  = false;
    trackedScene       = scene;

    if (!scene) return;
    EnsureDebugShaders();

    // Colour palette for spotlight indicators (cycles if >8 lights)
    static const ZNVector4 kPalette[] = {
        ZNVector4(1.f, 0.4f, 0.4f, 1.f),
        ZNVector4(0.4f, 1.f, 0.4f, 1.f),
        ZNVector4(0.4f, 0.6f, 1.f, 1.f),
        ZNVector4(1.f, 1.f, 0.3f, 1.f),
        ZNVector4(0.3f, 1.f, 1.f, 1.f),
        ZNVector4(1.f, 0.4f, 1.f, 1.f),
        ZNVector4(1.f, 0.7f, 0.2f, 1.f),
        ZNVector4(0.7f, 0.3f, 1.f, 1.f),
    };

    const auto& spots = scene->GetSpotLights();
    for (int i = 0; i < (int)spots.size(); ++i)
    {
        ZNSpotLight* light = spots[i];
        ZNVector4 col = kPalette[i % 8];
        float outerAngle = light->GetOuterCutoffAngle();
        ZNVector3 pos    = light->GetPosition();
        ZNVector3 dir    = light->GetDirection();

        SpotEntry e;
        e.light          = light;
        e.initOuterAngle = outerAngle;

        e.markerMat = ZNMaterialFactory::CreatePBR(dbgSolidShader,
            ZNVector4(col.x, col.y, col.z, 1.f), 0.f, 1.f);
        e.marker = new ZNGameObject();
        e.marker->SetMesh(ZNMeshFactory::CreateCube(0.1f));
        e.marker->GetMesh()->SetMaterial(e.markerMat);
        e.marker->SetMaterial(e.markerMat);
        e.marker->GetTransform().position = pos;
        e.marker->SetCastShadow(false);

        e.coneMat = ZNMaterialFactory::CreatePBR(dbgAlphaShader,
            ZNVector4(col.x, col.y, col.z, 0.2f), 0.f, 1.f);
        e.cone = new ZNGameObject();
        e.cone->SetMesh(ZNMeshFactory::CreateConeFromApex(outerAngle, 5.f, 32));
        e.cone->GetMesh()->SetMaterial(e.coneMat);
        e.cone->SetMaterial(e.coneMat);
        e.cone->GetTransform().position = pos;
        float zRot = atan2f(-dir.x, -dir.y) * 180.f / kPI;
        float xRot = asinf((std::max)(-1.f, (std::min)(1.f, dir.z))) * 180.f / kPI;
        e.cone->GetTransform().rotation = ZNVector3(xRot, 0.f, zRot);
        e.cone->SetCastShadow(false);

        spotEntries.push_back(std::move(e));
    }

    // Camera indicators
    static const ZNVector4 kCamCol(0.f, 0.9f, 1.f, 1.f); // cyan
    for (const auto& dc : scene->GetDebugCameras())
    {
        ZNCamera* cam = dc.cam;
        CamEntry e;
        e.cam  = cam;
        e.name = dc.name;

        e.mat = ZNMaterialFactory::CreatePBR(dbgSolidShader, kCamCol, 0.f, 1.f);

        e.marker = new ZNGameObject();
        e.marker->SetMesh(ZNMeshFactory::CreateCube(0.12f));
        e.marker->GetMesh()->SetMaterial(e.mat);
        e.marker->SetMaterial(e.mat);
        e.marker->GetTransform().position = cam->GetPosition();
        e.marker->SetCastShadow(false);

        e.lens = new ZNGameObject();
        e.lens->SetMesh(ZNMeshFactory::CreateCube(0.06f));
        e.lens->GetMesh()->SetMaterial(e.mat);
        e.lens->SetMaterial(e.mat);
        e.lens->GetTransform().position = cam->GetPosition() + cam->GetForward() * 0.3f;
        e.lens->SetCastShadow(false);

        camEntries.push_back(std::move(e));
    }
}

void SceneDebugUI::UpdateDebugEntries()
{
    for (auto& e : spotEntries)
    {
        if (!e.light || !e.marker) continue;
        ZNVector3 pos = e.light->GetPosition();
        ZNVector3 dir = e.light->GetDirection();

        e.marker->GetTransform().position = pos;

        if (e.cone)
        {
            e.cone->GetTransform().position = pos;

            // Direction → cone rotation (same formula as manual setup)
            float zRot = atan2f(-dir.x, -dir.y) * 180.f / kPI;
            float xRot = asinf((std::max)(-1.f, (std::min)(1.f, dir.z))) * 180.f / kPI;
            e.cone->GetTransform().rotation = ZNVector3(xRot, 0.f, zRot);

            // Scale cone XZ to reflect current outer angle change (no mesh rebuild needed)
            float currOuter = e.light->GetOuterCutoffAngle();
            if (e.initOuterAngle > 0.01f && currOuter > 0.01f)
            {
                float scaleXZ = tanf(currOuter * kPI / 180.f) /
                                tanf(e.initOuterAngle * kPI / 180.f);
                e.cone->GetTransform().scale = ZNVector3(scaleXZ, 1.f, scaleXZ);
            }
        }
    }

    for (auto& e : camEntries)
    {
        if (!e.cam || !e.marker) continue;
        ZNVector3 pos = e.cam->GetPosition();
        ZNVector3 fwd = e.cam->GetForward();
        e.marker->GetTransform().position = pos;
        if (e.lens)
            e.lens->GetTransform().position = pos + fwd * 0.3f;
    }
}

void SceneDebugUI::RenderDebugEntries()
{
    if (showSpotIndicators)
    {
        for (auto& e : spotEntries)
        {
            if (e.marker) e.marker->Render();
            if (e.cone)   e.cone->Render();
        }
    }
    if (showCamIndicators)
    {
        for (auto& e : camEntries)
        {
            if (e.marker) e.marker->Render();
            if (e.lens)   e.lens->Render();
        }
    }
}

void SceneDebugUI::RenderDebugPanel(ZNScene* scene)
{
    if (!visible) return;

    ZNCommandQueue* cq = GraphicsContext::GetInstance().GetCommandQueue();
    ImGui::SetNextWindowSize(ImVec2(220.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug");

    // Wireframe (common to all scenes)
    bool wireframe = (cq->GetViewMode() == ViewMode::Wireframe);
    if (ImGui::Checkbox("Wireframe", &wireframe))
        cq->SetViewMode(wireframe ? ViewMode::Wireframe : ViewMode::Lit);

    // Spotlight indicators (shown only when scene has spotlights)
    if (!spotEntries.empty())
        ImGui::Checkbox("Spotlight Indicators", &showSpotIndicators);

    // Camera indicators (shown only when scene has registered debug cameras)
    if (!camEntries.empty())
        ImGui::Checkbox("Camera Indicators", &showCamIndicators);

    // Scene-specific extras (grid toggle, turntable, etc.)
    if (onDebugExtras)
    {
        ImGui::Separator();
        onDebugExtras();
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
// Main render
// ---------------------------------------------------------------------------

void SceneDebugUI::Render(ZNScene* scene)
{
    // Tick timing (always, so FPS stays accurate even when UI is hidden)
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
                cpuUsage = (1.f - float(idleDelta) / totalDelta) * 100.f;
            prevIdle   = idleU;
            prevKernel = kernelU;
            prevUser   = userU;
        }
    }

    // Detect scene change → rebuild debug overlay entries
    if (scene != trackedScene)
        OnSceneChanged(scene);

    // Update indicator positions/rotations/scales every frame
    UpdateDebugEntries();

    // Render indicator game objects directly (forward pass context is active)
    RenderDebugEntries();

    if (!visible) return;

    // --- Stats ---
    const float cpuMs        = fpsDisplay > 0.f ? 1000.f / fpsDisplay : 0.f;
    const float gpuMs        = GraphicsContext::GetInstance().GetCommandQueue()
                             ? GraphicsContext::GetInstance().GetCommandQueue()->GetGpuFrameTimeMs()
                             : 0.f;
    const float gpuMemUsedMB = GraphicsContext::GetInstance().GetDevice()
                             ? GraphicsContext::GetInstance().GetDevice()->GetGpuMemoryUsageMB()
                             : 0.f;
    const float gpuMemBudgMB = GraphicsContext::GetInstance().GetDevice()
                             ? GraphicsContext::GetInstance().GetDevice()->GetGpuMemoryBudgetMB()
                             : 0.f;

    int totalObjs = 0, visibleObjs = 0;
    if (scene)
    {
        for (auto* o : scene->GetGameObjects())
        {
            ++totalObjs;
            if (o->IsVisible()) ++visibleObjs;
        }
    }

    ImGui::SetNextWindowPos(ImVec2(10.f, 10.f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(220.f, 0.f), ImGuiCond_Always);
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
    ImGui::SetNextWindowSize(ImVec2(220.f, 400.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Outliner");

    if (ImGui::TreeNodeEx("GameObjects", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (scene)
        {
            auto showObj = [&](ZNGameObject* obj) {
                const auto& tag = obj->GetTag();
                if (tag == "Debug" || tag == "Room") return;
                bool isSelected = (selection.type == SelectionType::GameObject && selection.ptr == obj);
                if (ImGui::Selectable(obj->GetName().c_str(), isSelected))
                {
                    selection.type = SelectionType::GameObject;
                    selection.ptr  = obj;
                }
            };
            for (auto* obj : scene->GetGameObjects())
                showObj(obj);
            for (auto* obj : scene->GetForwardGameObjects())
                showObj(obj);
        }
        ImGui::TreePop();
    }

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
    ImGui::SetNextWindowSize(ImVec2(280.f, 400.f), ImGuiCond_FirstUseEver);
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
        if (ImGui::DragFloat3("Scale", sc, 0.001f, 0.001f, 100.f))
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
            if (ImGui::SliderFloat("Metallic",  &p.metallic,  0.f, 1.f)) changed = true;
            if (ImGui::SliderFloat("Roughness", &p.roughness, 0.f, 1.f)) changed = true;
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
        if (ImGui::SliderFloat("Intensity", &intensity, 0.f, 10.f))
            light->SetIntensity(intensity);
        ZNVector3 pos = light->GetPosition();
        float posArr[3] = { pos.x, pos.y, pos.z };
        if (ImGui::DragFloat3("Position", posArr, 0.05f))
            light->SetPosition(ZNVector3(posArr[0], posArr[1], posArr[2]));
        ZNVector3 dir = light->GetDirection();
        float dirArr[3] = { dir.x, dir.y, dir.z };
        if (ImGui::DragFloat3("Direction", dirArr, 0.01f, -1.f, 1.f))
        {
            ZNVector3 newDir(dirArr[0], dirArr[1], dirArr[2]);
            newDir.Normalize();
            light->SetDirection(newDir);
        }
        float innerAngle = light->GetInnerCutoffAngle();
        float outerAngle = light->GetOuterCutoffAngle();
        bool cutoffChanged = false;
        if (ImGui::SliderFloat("Inner Angle", &innerAngle, 0.f, 89.f)) cutoffChanged = true;
        if (ImGui::SliderFloat("Outer Angle", &outerAngle, 0.f, 89.f)) cutoffChanged = true;
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
        if (ImGui::SliderFloat("Intensity", &intensity, 0.f, 20.f))
            dl->SetIntensity(intensity);
        ZNVector3 ddir = dl->GetDirection();
        float dirArr[3] = { ddir.x, ddir.y, ddir.z };
        if (ImGui::SliderFloat3("Direction", dirArr, -1.f, 1.f))
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
    ImGui::SetNextWindowSize(ImVec2(180.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scenes");
    const auto& entries = SceneManager::Get().GetEntries();
    int current = SceneManager::Get().GetCurrentIndex();
    for (int i = 0; i < (int)entries.size(); i++)
    {
        bool isActive = (i == current);
        if (isActive)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.15f, 1.f));
        if (ImGui::Button(entries[i].name.c_str(), ImVec2(-1, 0)))
            SceneManager::Get().SwitchTo(i);
        if (isActive)
            ImGui::PopStyleColor();
    }
    ImGui::End();

    // --- Debug (common + scene-specific) ---
    RenderDebugPanel(scene);
}

void SceneDebugUI::RenderToggle()
{
    if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
        visible = !visible;

    auto& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 58.f, 10.f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(48.f, 30.f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 4.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));
    ImGui::Begin("##ui_toggle", nullptr,
        ImGuiWindowFlags_NoTitleBar      | ImGuiWindowFlags_NoResize    |
        ImGuiWindowFlags_NoScrollbar     | ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImVec4 btnColor = visible
        ? ImVec4(0.15f, 0.50f, 0.15f, 1.f)
        : ImVec4(0.45f, 0.15f, 0.15f, 1.f);
    ImVec4 hoverColor(btnColor.x + 0.1f, btnColor.y + 0.1f, btnColor.z + 0.1f, 1.f);
    ImGui::PushStyleColor(ImGuiCol_Button,        btnColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  hoverColor);
    if (ImGui::Button(visible ? " UI " : " -- ", ImVec2(40.f, 22.f)))
        visible = !visible;
    ImGui::PopStyleColor(3);

    ImGui::End();
    ImGui::PopStyleVar(2);
}
