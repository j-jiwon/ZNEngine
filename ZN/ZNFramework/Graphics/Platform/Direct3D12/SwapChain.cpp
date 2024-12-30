#include "SwapChain.h"

#include "ZNFramework.h"
#include "CommandQueue.h"
#include "GraphicsDevice.h"

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

void SwapChain::Resize(uint32_t inWidth, uint32_t inHeight)
{
    if (width != inWidth || height != inHeight)
    {
        width = inWidth;
        height = inHeight;

        for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
        {
            rtvBuffer[i].Reset();
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
            ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffer[i])));
        }

        // descriptor resize
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = rtvHeap->GetCPUDescriptorHandleForHeapStart();

        for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
            rtvHandle.Offset(i, rtvHeapSize);
            device->Device()->CreateRenderTargetView(rtvBuffer[i].Get(), nullptr, rtvHandle);
        }
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

void SwapChain::CreateSwapChainInternal()
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
        hwnd,
        &desc,
        nullptr,
        nullptr,
        swapChain1.GetAddressOf()));

    ThrowIfFailed(swapChain1.As(&swapChain));

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

