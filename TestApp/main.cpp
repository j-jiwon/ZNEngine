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
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Indicate a state transition on the resource usage.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &barrier);
    //mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
    //    D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
     
    // Specify the buffers we are going to render to.
    auto currentBackBufferView = CurrentBackBufferView();
    auto depthStencilView = DepthStencilView();
    mCommandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView);

    // Indicate a state transition on the resource usage.
    auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &barrier2);

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Wait until frame commands are complete.  This waiting is inefficient and is
    // done for simplicity.  Later we will show how to organize our rendering code
    // so we do not have to wait per frame.
    FlushCommandQueue();
}