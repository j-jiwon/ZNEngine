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
        CommandQueue(GraphicsDevice*, ID3D12CommandQueue*);
        ~CommandQueue() noexcept = default;

        ZNSwapChain* CreateSwapChain(const ZNWindow*) override;

        ID3D12CommandQueue* Queue() const { return queue.Get(); }

    private:
        ComPtr<ID3D12CommandQueue> queue;
        GraphicsDevice* device;
    };
}
