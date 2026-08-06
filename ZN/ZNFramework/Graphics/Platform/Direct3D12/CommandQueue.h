#pragma once
#include "Graphics/ZNCommandQueue.h"
#include "ZNUtils.h"
#include "RenderGraph.h"
#include "RenderTexture.h"
#include "CubeRenderTexture.h"
#include <functional>
#include <vector>
#include <string>

namespace ZNFramework {

class GraphicsDevice;
class SwapChain;
class GBufferManager;
class DeferredLightingPass;
class ShadowMap;
class BloomChain;
class IBLBaker;
class SkyboxRenderer;
class ZNCamera;
class ZNMatrix4;
namespace Platform::Direct3D { class ImGuiLayer; }

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
    const std::vector<GpuPassTiming>& GetGpuPassTimings() const override { return passGpuTimings; }

    GBufferManager*       GetGBufferManager()    { return gbufferManager; }
    ShadowMap*            GetShadowMap()         { return shadowMap; }
    RenderTexture*        GetSceneColorRT()      { return sceneColorRT; }
    BloomChain*           GetBloomChain()        { return bloomChain; }
    IBLBaker*             GetIBLBaker()          { return iblBaker; }
    SkyboxRenderer*       GetSkyboxRenderer()    { return skyboxRenderer; }

    void SetGBufferManager(GBufferManager* manager)            { gbufferManager = manager; }
    void SetDeferredLightingPass(DeferredLightingPass* pass)   { deferredLightingPass = pass; }
    void SetShadowMap(ShadowMap* shadow)                       { shadowMap = shadow; }
    void SetSceneColorRT(RenderTexture* rt)                    { sceneColorRT = rt; }
    void SetBloomChain(BloomChain* chain)                      { bloomChain = chain; }
    void SetIBLBaker(IBLBaker* baker)                          { iblBaker = baker; }
    void SetSkyboxRenderer(SkyboxRenderer* renderer)           { skyboxRenderer = renderer; }
    void SetShadowRenderCallback(std::function<void()> cb)     { shadowRenderCallback = std::move(cb); }
    void SetGBufferRenderCallback(std::function<void()> cb)    { gbufferRenderCallback = std::move(cb); }

    // Re-import SceneColor's resource pointer after a resize (resource is recreated)
    void RefreshSceneColorResource();

    // Re-import Bloom's (mip0) resource pointer after a resize (resource is recreated)
    void RefreshBloomChainResource();

    bool IsForwardPass() const { return isForwardPass; }

    // Dedicated per-frame upload buffer for forward-pass point lights.
    // Called from Material::Bind() during forward pass rendering.
    D3D12_CPU_DESCRIPTOR_HANDLE UpdateFwdPointLightBuffer(const void* data, uint32 size);

    // Per-frame ring buffer for GBuffer instanced-draw world matrices (see Mesh::RenderInstanced).
    // Writes `count` row-major float4x4s at the next free offset and returns an SRV over just that
    // range; offsets never overlap within a frame since the whole command list is recorded before
    // any of it executes (no frame pipelining), so an earlier batch's region must stay untouched
    // until the GPU actually reads it.
    D3D12_CPU_DESCRIPTOR_HANDLE PushInstanceWorlds(const ZNMatrix4* worlds, uint32 count);
    static constexpr uint32 kInstanceWorldCapacity        = 4096; // total instances/frame, all batches
    static constexpr uint32 kMaxInstanceBatchesPerFrame    = 32;  // distinct (mesh) batches/frame

    // Re-import GBuffer resource pointers after a resize (resources are recreated)
    void RefreshGBufferResources();

    // Kept for backwards compatibility; calls RefreshGBufferResources() internally
    void NotifyGBufferResized() { RefreshGBufferResources(); }

    RenderGraph& GetRenderGraph() { return renderGraph; }

    static constexpr uint32 kFwdPLBufSize = 512; // 256-aligned; fits 8 × 48B lights + 16B header

    // Off-screen camera: renders to a RenderTexture before the main passes.
    // resourceName identifies the texture in the RenderGraph (e.g. "CCTV").
    // Must be called before the first frame (before BuildRenderGraph runs).
    void AddOffscreenCamera(ZNCamera* cam, RenderTexture* rt,
                            const std::string& resourceName,
                            std::function<void()> cb)
    {
        offscreenCameras.push_back({ cam, rt, resourceName, std::move(cb) });
    }

    // The ImGui layer, so content (scenes) can turn an RT SRV into an ImGui thumbnail via its
    // SetTexture() — same machinery the engine uses for the GBuffer preview.
    void        SetImGuiLayer(Platform::Direct3D::ImGuiLayer* layer) { imguiLayer = layer; }
    Platform::Direct3D::ImGuiLayer* GetImGuiLayer() const { return imguiLayer; }

    // One-shot cubemap capture: renders into all 6 faces on the first frame only, then
    // the CubeRenderTexture is a stable static environment map. Must be called before the
    // first frame (before BuildRenderGraph runs). cb receives the face index (0-5) being
    // rendered, so callers can draw a per-face skybox background before scene geometry.
    void AddCubemapCapture(std::vector<ZNCamera*> cams, CubeRenderTexture* rt,
                           const std::string& resourceName,
                           std::function<void(uint32)> cb)
    {
        cubemapCaptures.push_back({ std::move(cams), rt, resourceName, std::move(cb) });
    }

    // The most recently captured (or currently-being-captured) environment cubemap, used by
    // the deferred lighting pass for metallic-surface reflections. Falls back to a black
    // cube (no reflection contribution) when no scene has registered one.
    void SetEnvCubemapSRV(D3D12_CPU_DESCRIPTOR_HANDLE handle) { envCubemapSRV = handle; hasEnvCubemap = true; }
    // Deactivates the env cubemap slot (e.g. the newly active scene never registered one) —
    // does not touch envCubemapSRV, so re-activating a scene that did doesn't need a re-set.
    void ClearEnvCubemapSRV() { hasEnvCubemap = false; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetEnvCubemapSRV() const { return envCubemapSRV; }
    bool HasEnvCubemap() const { return hasEnvCubemap; }

    // Visible background skybox (separate from the reflection env cubemap above — this one
    // is actually drawn where the depth buffer has no geometry, see deferred_lighting.hlsli).
    // Same single-slot-per-active-scene pattern as the env cubemap.
    void SetSkyboxSRV(D3D12_CPU_DESCRIPTOR_HANDLE handle) { skyboxSRV = handle; hasSkybox = true; }
    void ClearSkyboxSRV() { hasSkybox = false; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSkyboxSRV() const { return skyboxSRV; }
    bool HasSkybox() const { return hasSkybox; }

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
    ShadowMap*             shadowMap             = nullptr;
    RenderTexture*         sceneColorRT          = nullptr;
    BloomChain*            bloomChain            = nullptr;
    IBLBaker*              iblBaker              = nullptr;
    SkyboxRenderer*        skyboxRenderer        = nullptr;

    struct OffscreenCameraEntry {
        ZNCamera*       camera;
        RenderTexture*  output;
        std::string     resourceName;
        std::function<void()> renderCb;
    };

    struct CubemapCaptureEntry {
        std::vector<ZNCamera*>      cams;
        CubeRenderTexture*          output;
        std::string                 resourceName;
        std::function<void(uint32)> renderCb;
    };

    Platform::Direct3D::ImGuiLayer* imguiLayer = nullptr;

    std::function<void()> shadowRenderCallback;
    std::function<void()> gbufferRenderCallback;
    std::vector<OffscreenCameraEntry> offscreenCameras;
    std::vector<CubemapCaptureEntry>  cubemapCaptures;

    D3D12_CPU_DESCRIPTOR_HANDLE envCubemapSRV = {};
    bool                        hasEnvCubemap = false;

    D3D12_CPU_DESCRIPTOR_HANDLE skyboxSRV = {};
    bool                        hasSkybox = false;

    // Dedicated forward-pass point light upload buffer (not shared with transform/material CB)
    ComPtr<ID3D12Resource>       fwdPointLightBuffer;
    void*                        fwdPointLightMapped    = nullptr;
    ComPtr<ID3D12DescriptorHeap> fwdPointLightCBVHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE  fwdPointLightCBVHandle = {};

    bool isForwardPass    = false;
    bool renderGraphBuilt = false;
    bool isFirstFrame     = true;

    RenderGraph renderGraph;

    // GPU timestamp query (whole frame)
    ComPtr<ID3D12QueryHeap> timestampQueryHeap;
    ComPtr<ID3D12Resource>  timestampReadbackBuffer;
    UINT64 timestampFrequency = 0;
    float  gpuFrameTimeMs     = 0.0f;

    // GPU timestamp query (per RenderGraph pass). Sized once passes are known — right after
    // BuildRenderGraph() on the first frame — since pass count/order is fixed after that.
    ComPtr<ID3D12QueryHeap>    passTimestampQueryHeap;
    ComPtr<ID3D12Resource>     passTimestampReadbackBuffer;
    std::vector<GpuPassTiming> passGpuTimings;

    // GBuffer instanced-draw world matrix ring buffer (see PushInstanceWorlds).
    ComPtr<ID3D12Resource>       instanceWorldBuffer;
    void*                        instanceWorldMapped   = nullptr;
    ComPtr<ID3D12DescriptorHeap> instanceWorldSrvHeap;  // non-shader-visible; copied via TableDescriptorHeap::SetSRV
    uint32                       instanceWorldCursor    = 0;
    uint32                       instanceWorldSrvIndex  = 0;
};

} // namespace ZNFramework
