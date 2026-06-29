#pragma once
#include <functional>
#include <chrono>
#include <vector>
#include <string>

namespace ZNFramework {
    class ZNScene;
    class ZNCamera;
    class ZNSpotLight;
    class ZNPointLight;
    class ZNShader;
    class ZNGameObject;
    class ZNMaterial;
}

class SceneDebugUI
{
public:
    static SceneDebugUI& Get() { static SceneDebugUI s; return s; }

    enum class SelectionType { None, GameObject, SpotLight, PointLight, DirectionalLight, Custom };
    struct Selection {
        SelectionType type = SelectionType::None;
        void*         ptr  = nullptr;
    };

    // Render shared panels (Stats, Outliner, Inspector, Scenes, Debug). Call after scene's RenderForward().
    void Render(ZNFramework::ZNScene* scene);

    // Always-visible toggle button + F2 hotkey. Call unconditionally every frame.
    void RenderToggle();

    bool IsVisible()  const { return visible; }
    void Toggle()           { visible = !visible; }

    Selection&       GetSelection()       { return selection; }
    const Selection& GetSelection() const { return selection; }
    void             ClearSelection()     { selection = {}; }

    // Per-frame callbacks set by the active scene's RenderForward().
    std::function<void()>      onOutlinerExtras;   // extra tree nodes in Outliner
    std::function<void(void*)> onInspectorExtras;  // called when SelectionType::Custom selected
    std::function<void()>      onDebugExtras;      // scene-specific items in Debug panel

    // Debug-overlay state (readable for scenes that toggle them externally, e.g. F1 hotkey)
    bool showSpotIndicators = false;
    bool showCamIndicators  = false;

private:
    SceneDebugUI();

    // ---- Shared debug overlay (spotlight + camera indicators) ----
    struct SpotEntry {
        ZNFramework::ZNSpotLight*  light          = nullptr;
        ZNFramework::ZNGameObject* marker         = nullptr;
        ZNFramework::ZNGameObject* cone           = nullptr;
        ZNFramework::ZNMaterial*   markerMat      = nullptr;
        ZNFramework::ZNMaterial*   coneMat        = nullptr;
        float                      initOuterAngle = 0.f;
    };
    struct CamEntry {
        ZNFramework::ZNCamera*     cam    = nullptr;
        std::string                name;
        ZNFramework::ZNGameObject* marker = nullptr;
        ZNFramework::ZNGameObject* lens   = nullptr;
        ZNFramework::ZNMaterial*   mat    = nullptr;
    };

    void EnsureDebugShaders();
    void OnSceneChanged(ZNFramework::ZNScene* scene);
    void UpdateDebugEntries();
    void RenderDebugEntries();
    void RenderDebugPanel(ZNFramework::ZNScene* scene);

    ZNFramework::ZNShader* dbgSolidShader = nullptr;
    ZNFramework::ZNShader* dbgAlphaShader = nullptr;
    ZNFramework::ZNScene*  trackedScene   = nullptr;
    std::vector<SpotEntry> spotEntries;
    std::vector<CamEntry>  camEntries;

    // ---- UI state ----
    bool      visible    = true;
    Selection selection;

    float     fpsAccum   = 0.0f;
    int       fpsFrames  = 0;
    float     fpsDisplay = 0.0f;
    float     cpuUsage   = 0.0f;

    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point lastTime;

    unsigned long long prevIdle   = 0;
    unsigned long long prevKernel = 0;
    unsigned long long prevUser   = 0;
};
