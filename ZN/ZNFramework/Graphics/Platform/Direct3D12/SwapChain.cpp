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

void SwapChain::Resize(uint32 inWidth, uint32 inHeight)
{
    //if (width != inWidth || height != inHeight)
    //{
    //    width = inWidth;
    //    height = inHeight;

    //    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
    //    {
    //        rtvBuffer[i].Reset();
    //    }

    //    // 스왑 체인 버퍼 리사이즈
    //    ThrowIfFailed(swapChain->ResizeBuffers(
    //        FRAME_BUFFER_COUNT,          // 새로운 버퍼 수
    //        width,                       // 새로운 너비
    //        height,                      // 새로운 높이
    //        DXGI_FORMAT_R8G8B8A8_UNORM,  // 버퍼 포맷
    //        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH // 추가 플래그
    //    ));

    //    backBufferIndex = 0;

    //    // 새로운 렌더 타겟 뷰를 가져오기
    //    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
    //    {
    //        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffer[i])));
    //    }

    //    // descriptor resize
    //    D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = rtvHeap->GetCPUDescriptorHandleForHeapStart();

    //    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
    //    {
    //        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    //        rtvHandle.Offset(i, rtvHeapSize);
    //        device->Device()->CreateRenderTargetView(rtvBuffer[i].Get(), nullptr, rtvHandle);
    //    }
    //}
}

void SwapChain::Present()
{
    ThrowIfFailed(swapChain->Present(0, 0));
}

void SwapChain::SwapIndex()
{
    backBufferIndex = (backBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}

void SwapChain::CreateSwapChainInternal()
{
    swapChain.Reset();

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = static_cast<uint32>(width); // 버퍼의 해상도 너비
    sd.BufferDesc.Height = static_cast<uint32>(height); // 버퍼의 해상도 높이
    sd.BufferDesc.RefreshRate.Numerator = 60; // 화면 갱신 비율
    sd.BufferDesc.RefreshRate.Denominator = 1; // 화면 갱신 비율
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 버퍼의 디스플레이 형식
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = 1; // 멀티 샘플링 OFF
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 후면 버퍼에 렌더링할 것 
    sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT; // 전면+후면 버퍼
    sd.OutputWindow = hwnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 전면 후면 버퍼 교체 시 이전 프레임 정보 버림
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    device->Factory()->CreateSwapChain(queue->Queue(), &sd, &swapChain);

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

