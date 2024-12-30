#include "gtest/gtest.h"
#include "ZNFramework.h"
#include "ZNFramework/Window/Platform/WindowPlatform.h"
#include "ZNFramework/Graphics/Platform/GraphicsAPI.h"

using namespace ZNFramework;

class TestApp : public ZNApplication
{
public:
    TestApp()
        : window(nullptr)
    {}

    void OnInitialize() override;
    void OnTerminate() override;
    
private:
    ZNWindow* window = nullptr;
    ZNGraphicsDevice* device = nullptr;
};

int main()
{
    TestApp app;
    app.Run();
}

void TestApp::OnInitialize()
{
    // init main window
    window = ZNFramework::WindowPlatform::Create();
    window->Create();
    window->Show();

    device = ZNFramework::Platform::CreateGraphicsDevice();    
    context->Initialize(window, device);
};

void TestApp::OnTerminate()
{
}
