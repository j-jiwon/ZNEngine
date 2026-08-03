#include "SwapChain.h"

#include "ZNFramework.h"
#include "CommandQueue.h"
#include "GraphicsDevice.h"
#include <dxgi1_5.h> // IDXGIFactory5 + DXGI_FEATURE_PRESENT_ALLOW_TEARING

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
    ZNSwapChain* CreateSwapChain()
    {
        return new SwapChain();
    }
}

void SwapChain::Init(ZNCommandQueue* inQueue)
{
    // init member variables
    device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    queue = dynamic_cast<CommandQueue*>(inQueue);
    ZNWindow* window = WindowContext::GetInstance().GetWindow();
    hwnd = reinterpret_cast<HWND>(window->PlatformHandle());
    width = window->Width();
    height = window->Height();

    // initialize
    CreateSwapChainInternal();
    CreateRTV();
}

void SwapChain::Resize(uint32 inWidth, uint32 inHeight)
{
    if (inWidth == 0 || inHeight == 0)
        return;
    if (width == inWidth && height == inHeight)
        return;

    width = inWidth;
    height = inHeight;

    queue->WaitSync();

    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
        rtvBuffer[i].Reset();

    ThrowIfFailed(swapChain->ResizeBuffers(
        SWAP_CHAIN_BUFFER_COUNT,
        width,
        height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        swapChainFlags // must match creation (ALLOW_TEARING when supported)
    ));

    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
    {
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffer[i])));
        rtvHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), i * rtvHeapSize);
        device->Device()->CreateRenderTargetView(rtvBuffer[i].Get(), nullptr, rtvHandle[i]);
    }
}

void SwapChain::Present()
{
    // Sync interval 0 + ALLOW_TEARING (when supported) = uncapped present, no DWM vsync cap.
    HRESULT hr = swapChain->Present(0, presentFlags);

    // Window fully occluded (covered/minimized): DXGI didn't present. Not an error — just skip
    // and retry next frame.
    if (hr == DXGI_STATUS_OCCLUDED)
        return;

    // Device lost is unrecoverable at this layer (full device re-creation is out of scope) —
    // surface the removal reason rather than a bare "Present failed".
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        ThrowIfFailed(device->Device()->GetDeviceRemovedReason());
        ThrowIfFailed(hr);
    }

    ThrowIfFailed(hr);
}

void SwapChain::CreateSwapChainInternal()
{
    swapChain.Reset();

    // Detect tearing support (uncapped windowed present). Falls back to plain vsync present when
    // the adapter/OS doesn't support it, so both flags stay 0.
    {
        BOOL allowTearing = FALSE;
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(device->Factory().As(&factory5)) &&
            SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                    &allowTearing, sizeof(allowTearing))) &&
            allowTearing)
        {
            swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            presentFlags   = DXGI_PRESENT_ALLOW_TEARING;
        }
    }

    // Flip-model swap chain via CreateSwapChainForHwnd (the modern path; the legacy
    // CreateSwapChain + DXGI_SWAP_CHAIN_DESC is discouraged for flip effects).
    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width       = width;
    sd.Height      = height;
    sd.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.Stereo      = FALSE;
    sd.SampleDesc.Count   = 1;   // no MSAA on the back buffer (required for flip model)
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    sd.Scaling     = DXGI_SCALING_STRETCH;
    sd.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
    // ALLOW_TEARING when supported; no ALLOW_MODE_SWITCH — app owns all sizing (see MakeWindowAssociation).
    sd.Flags       = swapChainFlags;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(device->Factory()->CreateSwapChainForHwnd(
        queue->Queue(), hwnd, &sd, nullptr, nullptr, &swapChain1));

    // Stop DXGI from silently handling Alt+Enter / window-state changes (its automatic
    // fullscreen transitions are a common cause of maximize/present glitches). The app
    // drives every resize itself.
    device->Factory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

    ThrowIfFailed(swapChain1.As(&swapChain)); // QI to IDXGISwapChain3 for GetCurrentBackBufferIndex()

    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
    {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffer[i]));
    }
}

void SwapChain::CreateRTV()
{
    device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    rtvHeapSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDesc.NodeMask = 0;

    device->Device()->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap));
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
    {
        rtvHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * rtvHeapSize);
        device->Device()->CreateRenderTargetView(rtvBuffer[i].Get(), nullptr, rtvHandle[i]);
    }
}

