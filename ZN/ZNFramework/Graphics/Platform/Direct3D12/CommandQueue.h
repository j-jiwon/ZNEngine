#pragma once
#include "../../ZNCommandQueue.h"
#include "ZNUtils.h"
#include "GraphicsDevice.h"

namespace ZNFramework
{
    class CommandList;
    class GraphicsDevice;
    class CommandQueue : public ZNCommandQueue
    {
    public:
        CommandQueue(GraphicsDevice*, ID3D12CommandQueue*);
        ~CommandQueue() noexcept = default;

        ZNSwapChain* CreateSwapChain(const ZNWindow*) override;

        void Init() override;
        void RenderBegin() override;
        void RenderEnd() override;
        void OnResize(size_t width, size_t height) override;
        
        void WaitSync();
        void Enqueue(CommandList* commandList);

        ID3D12CommandQueue* Queue() const { return queue.Get(); }

    private:
        ComPtr<ID3D12CommandQueue> queue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;

        ComPtr<ID3D12Fence> fence;
        UINT fenceValue = 0;
        HANDLE fenceEvent = INVALID_HANDLE_VALUE;

        GraphicsDevice* device;
        class SwapChain* swapChain;
        class DescriptorHeap* descHeap;
    };
}
