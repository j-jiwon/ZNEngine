//#include <iostream>
//#include "Libs/googletest/include/gtest/gtest.h"
//
//#define USE_UNIT_TEST 1
//
//int main(int argc, char** argv)
//{
//#if USE_UNIT_TEST
//    ::testing::InitGoogleTest(&argc, argv);
//    return RUN_ALL_TESTS();
//#else
//    return 1;
//#endif
//}


#include "ZNFramework.h"
#include <DirectXColors.h>

using namespace DirectX;
using namespace ZNFramework;

class InitDirect3DApp : public ZNApplication
{
public:
    InitDirect3DApp(HINSTANCE hInstance);
    ~InitDirect3DApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const ZNTimer& gt)override;
    virtual void Draw(const ZNTimer& gt)override;

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
                    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        InitDirect3DApp theApp(hInstance);
        if (!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch (...)
    {
        MessageBox(nullptr, L"", L"HR Failed", MB_OK);
        return 0;
    }
}


InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
    : ZNApplication(hInstance)
{
}

InitDirect3DApp::~InitDirect3DApp()
{
}

bool InitDirect3DApp::Initialize()
{
    if (!ZNApplication::Initialize())
        return false;

    return true;
}

void InitDirect3DApp::OnResize()
{
    ZNApplication::OnResize();
}

void InitDirect3DApp::Update(const ZNTimer& gt)
{

}

void InitDirect3DApp::Draw(const ZNTimer& gt)
{
    ThrowIfFailed(commandAllocator->Reset());
    ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    ZNColor color = ZNColor(1, 1, 0, 0.5); // TODO Color -> ZNColor
    commandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::AliceBlue, 0, nullptr);
    commandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
     
    auto currentBackBufferView = CurrentBackBufferView();
    auto depthStencilView = DepthStencilView();
    commandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView);

    auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier2);

    ThrowIfFailed(commandList->Close());

    ID3D12CommandList* cmdsLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    ThrowIfFailed(swapChain->Present(0, 0));
    currBackBuffer = (currBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();
}