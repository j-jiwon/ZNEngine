#include "ZNFramework.h"
#include "ZNFramework/Window/Platform/WindowPlatform.h"
#include "ZNFramework/Graphics/Platform/GraphicsAPI.h"
#include "TestGameScene.h"
#include "CCTVScene.h"
#include "MirrorBallScene.h"
#include "SceneManager.h"
#include "SceneDebugUI.h"

using namespace ZNFramework;

class TestApp : public ZNApplication
{
public:
    TestApp() = default;

    void OnInitialize() override;
    void OnTerminate()  override;

private:
    void SwitchToScene(int index);

    ZNWindow*         window    = nullptr;
    ZNGraphicsDevice* device    = nullptr;
    int               activeIdx = -1;

    struct SceneSlot {
        std::string name;
        ZNScene*    instance = nullptr;
    };
    std::vector<SceneSlot> slots;
};

int main()
{
    TestApp app;
    app.Run();
}

void TestApp::OnInitialize()
{
    window = ZNFramework::WindowPlatform::Create();
    window->Create(1100, 800);

    device = ZNFramework::Platform::CreateGraphicsDevice();
    context->Initialize(window, device);

    // Eagerly initialise ALL scenes before the first frame so every scene's
    // AddOffscreenCamera() is registered before BuildRenderGraph() runs.
    {
        auto* s = new TestGameScene();
        s->Initialize();
        slots.push_back({ "Test Scene", s });
    }
    {
        auto* s = new CCTVScene();
        s->Initialize();
        slots.push_back({ "CCTV Scene", s });
    }
    {
        auto* s = new MirrorBallScene();
        s->Initialize();
        slots.push_back({ "MirrorBall Scene", s });
    }

    // Wire up SceneManager so ImGui buttons can trigger switches
    std::vector<std::string> names;
    for (auto& slot : slots) names.push_back(slot.name);
    SceneManager::Get().RegisterScenes(std::move(names),
        [this](int idx) { SwitchToScene(idx); });

    SwitchToScene(0);
}

void TestApp::OnTerminate()
{
    for (auto& slot : slots)
        delete slot.instance;
    slots.clear();
}

void TestApp::SwitchToScene(int index)
{
    if (index == activeIdx || index < 0 || index >= (int)slots.size())
        return;

    // Reset scene-specific UI state from the previous scene.
    SceneDebugUI::Get().onOutlinerExtras  = nullptr;
    SceneDebugUI::Get().onInspectorExtras = nullptr;
    SceneDebugUI::Get().ClearSelection();

    context->SetScene(slots[index].instance);
    activeIdx = index;
    SceneManager::Get().SetCurrentIndex(index);

    // Chain shared debug UI after the scene's own RenderForward.
    // context->SetScene() already set the forward callback; we override it here
    // to append SceneDebugUI::Render() and the always-visible toggle button.
    auto* scene = slots[index].instance;
    GraphicsContext::GetInstance().GetCommandQueue()->SetForwardRenderCallback([scene]() {
        if (scene) scene->RenderForward();
        SceneDebugUI::Get().Render(scene);
        SceneDebugUI::Get().RenderToggle();
    });
}
