#pragma once
#include "Graphics/ZNCommandQueue.h"
#include "ZNUtils.h"

namespace ZNFramework
{
    class GraphicsDevice;
    class SwapChain;
    class GBufferManager;
    class DeferredLightingPass;
    class DebugViewportRenderer;
    class CommandQueue : public ZNCommandQueue
    {
    public:
        CommandQueue() = default;
        ~CommandQueue() noexcept = default;

        void Init(class ZNSwapChain* inSwapChain) override;
        void RenderBegin() override;
        void RenderEnd() override;
        void FlushResourceQueue() override;
        void WaitSync() override;

        ID3D12CommandQueue* Queue() const { return queue.Get(); }
        ID3D12GraphicsCommandList* CommandList() { return commandList.Get(); }
        ID3D12GraphicsCommandList* ResourceCommandList() { return resourceCommandList.Get(); }

        GBufferManager* GetGBufferManager() { return gbufferManager; }
        void SetGBufferManager(GBufferManager* manager) { gbufferManager = manager; }
        void SetDeferredLightingPass(DeferredLightingPass* pass) { deferredLightingPass = pass; }
        void SetDebugViewportRenderer(DebugViewportRenderer* renderer) { debugViewportRenderer = renderer; }
        void SetGBufferEnabled(bool enabled) { enableGBuffer = enabled; }
        bool IsForwardPass() const { return isForwardPass; }
        void SetForwardPass(bool forward) { isForwardPass = forward; }

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

        GBufferManager* gbufferManager = nullptr;
        DeferredLightingPass* deferredLightingPass = nullptr;
        DebugViewportRenderer* debugViewportRenderer = nullptr;
        bool enableGBuffer = true; // Enable MRT by default
        bool isFirstFrame = true; // Track first frame for resource state
        bool isForwardPass = false; // Track if currently in forward pass
    };
}
