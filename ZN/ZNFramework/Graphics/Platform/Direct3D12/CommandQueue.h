#pragma once
#include "../../ZNCommandQueue.h"
#include "ZNUtils.h"
#include "GraphicsDevice.h"

namespace ZNFramework
{
    class GraphicsDevice;
    class CommandQueue : public ZNCommandQueue
    {
    public:
        CommandQueue() = default;
        ~CommandQueue() noexcept = default;

        void Init(class ZNSwapChain* inSwapChain) override;
        void RenderBegin() override;
        void RenderEnd() override;

        void WaitSync();

        ID3D12CommandQueue* Queue() const { return queue.Get(); }
        ID3D12GraphicsCommandList* CommandList() { return commandList.Get(); }

    private:
        ComPtr<ID3D12CommandQueue> queue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;

        ComPtr<ID3D12Fence> fence;
        UINT fenceValue = 0;
        HANDLE fenceEvent = INVALID_HANDLE_VALUE;

        GraphicsDevice* device;
        class SwapChain* swapChain;
    };
}
