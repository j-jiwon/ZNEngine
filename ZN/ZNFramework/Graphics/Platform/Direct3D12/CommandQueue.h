#pragma once
#include "Graphics/ZNCommandQueue.h"
#include "ZNUtils.h"
#include "RenderGraph.h"
#include "RenderTexture.h"
#include <functional>
#include <vector>
#include <string>

namespace ZNFramework {

class GraphicsDevice;
class SwapChain;
class GBufferManager;
class DeferredLightingPass;
class DebugViewportRenderer;
class ShadowMap;
class ZNCamera;

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

    ID3D12CommandQueue*        Queue()               const { return queue.Get(); }
    ID3D12GraphicsCommandList* CommandList()               { return commandList.Get(); }
    ID3D12GraphicsCommandList* ResourceCommandList()       { return resourceCommandList.Get(); }

    float GetGpuFrameTimeMs() const override { return gpuFrameTimeMs; }

    GBufferManager*       GetGBufferManager()    { return gbufferManager; }
    DebugViewportRenderer* GetDebugViewportRenderer() { return debugViewportRenderer; }
    ShadowMap*            GetShadowMap()         { return shadowMap; }

    void SetGBufferManager(GBufferManager* manager)            { gbufferManager = manager; }
    void SetDeferredLightingPass(DeferredLightingPass* pass)   { deferredLightingPass = pass; }
    void SetDebugViewportRenderer(DebugViewportRenderer* r)    { debugViewportRenderer = r; }
    void SetShadowMap(ShadowMap* shadow)                       { shadowMap = shadow; }
    void SetShadowRenderCallback(std::function<void()> cb)     { shadowRenderCallback = std::move(cb); }
    void SetGBufferRenderCallback(std::function<void()> cb)    { gbufferRenderCallback = std::move(cb); }

    bool IsForwardPass() const { return isForwardPass; }

    // Re-import GBuffer resource pointers after a resize (resources are recreated)
    void RefreshGBufferResources();

    // Kept for backwards compatibility; calls RefreshGBufferResources() internally
    void NotifyGBufferResized() { RefreshGBufferResources(); }

    RenderGraph& GetRenderGraph() { return renderGraph; }

    // Off-screen camera: renders to a RenderTexture before the main passes.
    // resourceName identifies the texture in the RenderGraph (e.g. "CCTV").
    // Must be called before the first frame (before BuildRenderGraph runs).
    void AddOffscreenCamera(ZNCamera* cam, RenderTexture* rt,
                            const std::string& resourceName,
                            std::function<void()> cb)
    {
        offscreenCameras.push_back({ cam, rt, resourceName, std::move(cb) });
    }

private:
    void BuildRenderGraph();

    ComPtr<ID3D12CommandQueue>        queue;
    ComPtr<ID3D12CommandAllocator>    commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;

    ComPtr<ID3D12CommandAllocator>    resourceCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> resourceCommandList;

    ComPtr<ID3D12Fence> fence;
    uint32              fenceValue = 0;
    HANDLE              fenceEvent = INVALID_HANDLE_VALUE;

    GraphicsDevice* device    = nullptr;
    SwapChain*      swapChain = nullptr;

    GBufferManager*        gbufferManager       = nullptr;
    DeferredLightingPass*  deferredLightingPass  = nullptr;
    DebugViewportRenderer* debugViewportRenderer = nullptr;
    ShadowMap*             shadowMap             = nullptr;

    struct OffscreenCameraEntry {
        ZNCamera*       camera;
        RenderTexture*  output;
        std::string     resourceName;
        std::function<void()> renderCb;
    };

    std::function<void()> shadowRenderCallback;
    std::function<void()> gbufferRenderCallback;
    std::vector<OffscreenCameraEntry> offscreenCameras;

    bool isForwardPass    = false;
    bool renderGraphBuilt = false;
    bool isFirstFrame     = true;

    RenderGraph renderGraph;

    // GPU timestamp query
    ComPtr<ID3D12QueryHeap> timestampQueryHeap;
    ComPtr<ID3D12Resource>  timestampReadbackBuffer;
    UINT64 timestampFrequency = 0;
    float  gpuFrameTimeMs     = 0.0f;
};

} // namespace ZNFramework
