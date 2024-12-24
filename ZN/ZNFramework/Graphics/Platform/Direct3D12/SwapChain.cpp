#include "SwapChain.h"
#include "../../../Window/ZNWindow.h"
#include "CommandQueue.h"
#include "Texture.h"

using namespace ZNFramework;

SwapChain::SwapChain(GraphicsDevice* device, CommandQueue* queue, const ZNWindow* window)
    : width(window->Width())
    , height(window->Height())
    , swapChain()
    , device(device)
    , queue(queue)
{
    window1 = reinterpret_cast<HWND>(window->PlatformHandle());
}

void SwapChain::Init()
{
    swapChain.Reset();

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = width;
    desc.Height = height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferCount = FRAME_BUFFER_COUNT;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Scaling = DXGI_SCALING_NONE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain1> swapChain1;

    ThrowIfFailed(device->Factory()->CreateSwapChainForHwnd(
        queue->Queue(),
        window1,
        &desc,
        nullptr,
        nullptr,
        swapChain1.GetAddressOf()));

    ThrowIfFailed(swapChain1.As(&swapChain));

    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
    {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
    }
}

void SwapChain::Present()
{
    ThrowIfFailed(swapChain->Present(1, 0));
}

void SwapChain::SwapIndex()
{
    backBufferIndex = (backBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}

void SwapChain::Resize(uint32_t inWidth, uint32_t inHeight)
{
    if (width != inWidth || height != inHeight)
    {
        width = inWidth;
        height = inHeight;

        for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
        {
            renderTargets[i].Reset();
        }

        // 스왑 체인 버퍼 리사이즈
        ThrowIfFailed(swapChain->ResizeBuffers(
            FRAME_BUFFER_COUNT,          // 새로운 버퍼 수
            width,                       // 새로운 너비
            height,                      // 새로운 높이
            DXGI_FORMAT_R8G8B8A8_UNORM,  // 버퍼 포맷
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH // 추가 플래그
        ));

        backBufferIndex = 0;

        // 새로운 렌더 타겟 뷰를 가져오기
        for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
        {
            ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
        }
    }
}

/*
if (this->width != width || this->height != height)
{
    this->width = width;
    this->height = height;

    // Clear resources
    //for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
    //{
    //    colorTexture[i]->resource = nullptr;
    //}
    //depthStencilTexture->resource = nullptr;

    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
    {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
    }
    device->depthstencilBuffer.Reset();

    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    ThrowIfFailed(swapChain->ResizeBuffers(
        FRAME_BUFFER_COUNT,
        width, height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    // Reset the frame index to the current back buffer index.

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(device->RtvHeap()->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < FRAME_BUFFER_COUNT; ++i)
    {
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&device->swapchainBuffer[i])));
        device->Device()->CreateRenderTargetView(device->swapchainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, rtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(device->depthstencilBuffer.GetAddressOf())
    ));

    device->Device()->CreateDepthStencilView(device->depthstencilBuffer.Get(), nullptr, device->DsvHeap()->GetCPUDescriptorHandleForHeapStart());
}
*/