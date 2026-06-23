#pragma once
#include <functional>
#include <chrono>

namespace ZNFramework { class ZNScene; }

class SceneDebugUI
{
public:
    static SceneDebugUI& Get() { static SceneDebugUI s; return s; }

    enum class SelectionType { None, GameObject, SpotLight, DirectionalLight, Custom };
    struct Selection {
        SelectionType type = SelectionType::None;
        void*         ptr  = nullptr;
    };

    // Render shared panels (Stats, Outliner, Inspector, Scenes). Call after scene's RenderForward().
    void Render(ZNFramework::ZNScene* scene);

    // Always-visible toggle button + F2 hotkey. Call unconditionally every frame.
    void RenderToggle();

    bool IsVisible()  const { return visible; }
    void Toggle()           { visible = !visible; }

    Selection&       GetSelection()       { return selection; }
    const Selection& GetSelection() const { return selection; }
    void             ClearSelection()     { selection = {}; }

    // Set by the active scene's RenderForward() every frame to inject scene-specific UI.
    std::function<void()>      onOutlinerExtras;   // e.g. "Cameras" tree node
    std::function<void(void*)> onInspectorExtras;  // called when SelectionType::Custom is selected

private:
    SceneDebugUI();

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
