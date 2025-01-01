#pragma once
#include "Graphics/ZNCommandQueue.h"
#include "ZNUtils.h"

namespace ZNFramework
{
    class GraphicsDevice;
    class SwapChain;
    class CommandQueue : public ZNCommandQueue
    {
    public:
        CommandQueue() = default;
        ~CommandQueue() noexcept = default;

        void Init(class ZNSwapChain* inSwapChain) override;
        void RenderBegin() override;
        void RenderEnd() override;
        void FlushResourceQueue() override;

        void WaitSync();

        ID3D12CommandQueue* Queue() const { return queue.Get(); }
        ID3D12GraphicsCommandList* CommandList() { return commandList.Get(); }
        ID3D12GraphicsCommandList* ResourceCommandList() { return resourceCommandList.Get(); }

    private:
        ComPtr<ID3D12CommandQueue> queue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;

        // resource
        ComPtr<ID3D12CommandAllocator> resourceCommandAllocator;
        ComPtr<ID3D12GraphicsCommandList> resourceCommandList;

        ComPtr<ID3D12Fence> fence;
        uint32 fenceValue = 0;
        HANDLE fenceEvent = INVALID_HANDLE_VALUE;

        GraphicsDevice* device;
        SwapChain* swapChain;
    };
}
