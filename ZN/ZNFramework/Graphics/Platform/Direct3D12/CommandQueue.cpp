#include "CommandQueue.h"
#include "SwapChain.h"
#include "RootSignature.h"
#include "ConstantBuffer.h"
#include "GraphicsDevice.h"
#include "TableDescriptorHeap.h"
#include "DepthStencilBuffer.h"
#include "GBufferManager.h"
#include "DeferredLightingPass.h"
#include "DebugViewportRenderer.h"
#include "ShadowMap.h"
#include "BloomChain.h"
#include "IBLBaker.h"
#include "SkyboxRenderer.h"
#include "Passes/ShadowPass.h"
#include "Passes/GBufferPass.h"
#include "Passes/DeferredLightingRenderPass.h"
#include "Passes/ForwardRenderPass.h"
#include "Passes/BloomPass.h"
#include "Passes/ToneMappingPass.h"
#include "Passes/ImGuiRenderPass.h"
#include "Passes/OffscreenCameraPass.h"
#include "Passes/CubeCapturePass.h"
#include "Passes/IBLBakePass.h"
#include "Passes/SkyboxPass.h"
#include "ZNFramework.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D {
    ZNCommandQueue* CreateCommandQueue() { return new CommandQueue(); }
}

void CommandQueue::Init(ZNSwapChain* inSwapChain)
{
    device    = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    swapChain = dynamic_cast<SwapChain*>(inSwapChain);

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    device->Device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
    device->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    device->Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
    commandList->Close();

    device->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&resourceCommandAllocator));
    device->Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, resourceCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&resourceCommandList));

    device->Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

    D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
    queryHeapDesc.Type  = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    queryHeapDesc.Count = 2;
    device->Device()->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&timestampQueryHeap));

    D3D12_HEAP_PROPERTIES readbackHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    D3D12_RESOURCE_DESC   readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(2 * sizeof(UINT64));
    device->Device()->CreateCommittedResource(
        &readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&timestampReadbackBuffer));

    queue->GetTimestampFrequency(&timestampFrequency);

    // Dedicated forward-pass point light buffer — 512B, permanently mapped
    {
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC   bufDesc   = CD3DX12_RESOURCE_DESC::Buffer(kFwdPLBufSize);
        device->Device()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&fwdPointLightBuffer));
        fwdPointLightBuffer->Map(0, nullptr, &fwdPointLightMapped);

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // CPU-only, gets copied to table heap
        device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&fwdPointLightCBVHeap));

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = fwdPointLightBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes    = kFwdPLBufSize; // 512 is a multiple of 256 ✓
        fwdPointLightCBVHandle = fwdPointLightCBVHeap->GetCPUDescriptorHandleForHeapStart();
        device->Device()->CreateConstantBufferView(&cbvDesc, fwdPointLightCBVHandle);
    }
}

void CommandQueue::BuildRenderGraph()
{
    // Import all tracked resources with their initial D3D12 resource states.
    // GBuffer textures start as RENDER_TARGET (just created / just resized).
    // Shadow map starts as DEPTH_WRITE (fresh depth buffer).
    // Back buffer starts as PRESENT (swap chain initialises it that way).
    if (shadowMap)
        renderGraph.Import("ShadowMap", shadowMap->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

    if (gbufferManager) {
        renderGraph.Import("GBuf_BaseColor", gbufferManager->GetBaseColorResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        renderGraph.Import("GBuf_Normal",    gbufferManager->GetNormalResource(),    D3D12_RESOURCE_STATE_RENDER_TARGET);
        renderGraph.Import("GBuf_DepthCopy", gbufferManager->GetDepthCopyResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        renderGraph.Import("GBuf_WorldPos",  gbufferManager->GetWorldPosResource(),  D3D12_RESOURCE_STATE_RENDER_TARGET);
        renderGraph.Import("GBuf_ARM",       gbufferManager->GetARMResource(),       D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    renderGraph.Import("BackBuffer", swapChain->GetBackRTVBuffer().Get(), D3D12_RESOURCE_STATE_PRESENT);

    if (sceneColorRT)
        renderGraph.Import("SceneColor", sceneColorRT->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    if (bloomChain)
        renderGraph.Import("Bloom", bloomChain->GetOutputResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Get concrete types needed by the passes
    auto* rootSig  = GraphicsContext::GetInstance().GetAs<RootSignature>();
    auto* tdh      = GraphicsContext::GetInstance().GetAs<TableDescriptorHeap>();
    auto* dsBuffer = GraphicsContext::GetInstance().GetAs<DepthStencilBuffer>();
    ZNShader* gbufShader = GraphicsContext::GetInstance().GetGBufferShader();

    // --- Shadow pass (runs first so offscreen cameras can sample the shadow map) ---
    if (shadowMap) {
        renderGraph.AddPass(std::make_unique<ShadowPass>(
            shadowMap,
            [this]() { if (shadowRenderCallback) shadowRenderCallback(); }));
    }

    // --- Offscreen camera passes (shadow map is now in PIXEL_SHADER_RESOURCE state) ---
    for (auto& entry : offscreenCameras) {
        // Start as RENDER_TARGET (the resource is created in that state)
        renderGraph.Import(entry.resourceName, entry.output->GetResource(),
                           D3D12_RESOURCE_STATE_RENDER_TARGET);
        renderGraph.AddPass(std::make_unique<OffscreenCameraPass>(
            entry.resourceName,
            entry.camera,
            entry.output,
            rootSig->GetSignature().Get(),
            tdh->GetDescriptorHeap().Get(),
            isForwardPass,
            entry.renderCb));
    }

    // --- Cubemap captures (static, one-shot on first frame) ---
    for (auto& entry : cubemapCaptures) {
        renderGraph.Import(entry.resourceName, entry.output->GetResource(),
                           D3D12_RESOURCE_STATE_RENDER_TARGET);
        renderGraph.AddPass(std::make_unique<CubeCapturePass>(
            entry.resourceName,
            entry.cams,
            entry.output,
            rootSig->GetSignature().Get(),
            tdh->GetDescriptorHeap().Get(),
            isForwardPass,
            entry.renderCb));
    }

    // --- IBL bake (BRDF LUT always; irradiance/prefiltered once an env cubemap is
    // active) — runs before GBuffer/DeferredLighting so this frame's lighting already
    // sees fresh data, same as CubeCapturePass above it. ---
    if (iblBaker) {
        renderGraph.AddPass(std::make_unique<IBLBakePass>(iblBaker, this));
    }

    // --- GBuffer pass (scene geometry) ---
    if (gbufferManager) {
        renderGraph.AddPass(std::make_unique<GBufferPass>(
            gbufferManager, gbufShader, dsBuffer, swapChain,
            [this]() { if (gbufferRenderCallback) gbufferRenderCallback(); }));
    }

    // --- Deferred lighting pass (writes HDR SceneColor) ---
    if (deferredLightingPass && gbufferManager && sceneColorRT) {
        renderGraph.AddPass(std::make_unique<DeferredLightingRenderPass>(
            deferredLightingPass, gbufferManager, shadowMap, swapChain, sceneColorRT));
    }

    // --- Skybox pass (fills SceneColor's background pixels, depth == far) ---
    if (sceneColorRT && gbufferManager && skyboxRenderer) {
        renderGraph.AddPass(std::make_unique<SkyboxPass>(
            skyboxRenderer, gbufferManager, sceneColorRT, this));
    }

    // --- Bloom pass (bright-pass extract + downsample/upsample chain from SceneColor) ---
    if (sceneColorRT && bloomChain) {
        renderGraph.AddPass(std::make_unique<BloomPass>(sceneColorRT, bloomChain));
    }

    // --- Tone mapping pass (HDR SceneColor + Bloom -> LDR BackBuffer, ACES + gamma) ---
    if (sceneColorRT && bloomChain) {
        ZNShader* toneMapShader = GraphicsContext::GetInstance().GetToneMapShader();
        renderGraph.AddPass(std::make_unique<ToneMappingPass>(
            sceneColorRT, bloomChain->GetOutputRT(), swapChain, toneMapShader));
    }

    // --- Forward pass ---
    renderGraph.AddPass(std::make_unique<ForwardRenderPass>(
        swapChain, dsBuffer,
        rootSig->GetSignature().Get(),
        tdh->GetDescriptorHeap().Get(),
        isForwardPass,
        [this]() { if (forwardRenderCallback) forwardRenderCallback(); }));

    // --- ImGui pass ---
    // Pass &imguiSrvHeap so that a late SetImGuiDescriptorHeap() call is picked up automatically
    renderGraph.AddPass(std::make_unique<ImGuiRenderPass>(
        &imguiSrvHeap,
        [this]() { if (imguiRenderCallback) imguiRenderCallback(); }));
}

D3D12_CPU_DESCRIPTOR_HANDLE CommandQueue::UpdateFwdPointLightBuffer(const void* data, uint32 size)
{
    if (fwdPointLightMapped && size <= kFwdPLBufSize)
        memcpy(fwdPointLightMapped, data, size);
    return fwdPointLightCBVHandle;
}

void CommandQueue::RefreshGBufferResources()
{
    if (!gbufferManager || !renderGraphBuilt) return;

    // After GBufferManager::Resize() the old D3D12 resources are released and new ones created.
    // Re-import so the RenderGraph state tracker points at the new resources.
    renderGraph.Import("GBuf_BaseColor", gbufferManager->GetBaseColorResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    renderGraph.Import("GBuf_Normal",    gbufferManager->GetNormalResource(),    D3D12_RESOURCE_STATE_RENDER_TARGET);
    renderGraph.Import("GBuf_DepthCopy", gbufferManager->GetDepthCopyResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    renderGraph.Import("GBuf_WorldPos",  gbufferManager->GetWorldPosResource(),  D3D12_RESOURCE_STATE_RENDER_TARGET);
    renderGraph.Import("GBuf_ARM",       gbufferManager->GetARMResource(),       D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void CommandQueue::RefreshSceneColorResource()
{
    if (!sceneColorRT || !renderGraphBuilt) return;

    // After RenderTexture::Resize() the old D3D12 resource is released and a new one created.
    renderGraph.Import("SceneColor", sceneColorRT->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void CommandQueue::RefreshBloomChainResource()
{
    if (!bloomChain || !renderGraphBuilt) return;

    // After BloomChain::Resize() mip0's old D3D12 resource is released and a new one created.
    renderGraph.Import("Bloom", bloomChain->GetOutputResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void CommandQueue::RenderBegin()
{
    // Build the graph lazily on the first frame (all callbacks are set by then)
    if (!renderGraphBuilt) {
        BuildRenderGraph();
        renderGraphBuilt = true;
    }

    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), nullptr);

    if (timestampQueryHeap)
        commandList->EndQuery(timestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);

    // Global per-frame setup: root signature, constant buffer, table descriptor heap
    RootSignature* rootSignature = GraphicsContext::GetInstance().GetAs<RootSignature>();
    commandList->SetGraphicsRootSignature(rootSignature->GetSignature().Get());

    ConstantBuffer* constantBuffer = GraphicsContext::GetInstance().GetAs<ConstantBuffer>();
    constantBuffer->Clear();

    TableDescriptorHeap* tableDescHeap = GraphicsContext::GetInstance().GetAs<TableDescriptorHeap>();
    tableDescHeap->Clear();
    ID3D12DescriptorHeap* descHeap = tableDescHeap->GetDescriptorHeap().Get();
    commandList->SetDescriptorHeaps(1, &descHeap);

    // Update back buffer pointer — it changes every frame after SwapIndex()
    renderGraph.UpdateResource("BackBuffer", swapChain->GetBackRTVBuffer().Get());

    // Execute all render passes (shadow → gbuffer → deferred lighting → forward → imgui)
    renderGraph.Execute(commandList.Get());
}

void CommandQueue::RenderEnd()
{
    if (timestampQueryHeap) {
        commandList->EndQuery(timestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
        commandList->ResolveQueryData(timestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
            0, 2, timestampReadbackBuffer.Get(), 0);
    }

    // Final transition: back buffer must be PRESENT before the swap
    RGResource* backBuffer = renderGraph.GetResource("BackBuffer");
    renderGraph.Transition(commandList.Get(), backBuffer, D3D12_RESOURCE_STATE_PRESENT);

    ThrowIfFailed(commandList->Close());

    ID3D12CommandList* cmdListArr[] = { commandList.Get() };
    queue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

    swapChain->Present();
    WaitSync();
    // Back-buffer index is now queried live from DXGI (SwapChain::GetCurrentBackBufferIndex),
    // so no manual index advance is needed here anymore.

    if (!isFirstFrame && timestampQueryHeap && timestampFrequency > 0) {
        void* pData = nullptr;
        D3D12_RANGE readRange = { 0, 2 * sizeof(UINT64) };
        timestampReadbackBuffer->Map(0, &readRange, &pData);
        const UINT64* ts = reinterpret_cast<const UINT64*>(pData);
        gpuFrameTimeMs = static_cast<float>(ts[1] - ts[0]) / static_cast<float>(timestampFrequency) * 1000.0f;
        D3D12_RANGE writeRange = { 0, 0 };
        timestampReadbackBuffer->Unmap(0, &writeRange);
    }

    if (isFirstFrame) isFirstFrame = false;
}

void CommandQueue::FlushResourceQueue()
{
    resourceCommandList->Close();

    ID3D12CommandList* commandListArray[] = { resourceCommandList.Get() };
    queue->ExecuteCommandLists(_countof(commandListArray), commandListArray);

    WaitSync();

    resourceCommandAllocator->Reset();
    resourceCommandList->Reset(resourceCommandAllocator.Get(), nullptr);
}

void CommandQueue::WaitSync()
{
    fenceValue++;
    queue->Signal(fence.Get(), fenceValue);

    if (fence->GetCompletedValue() < fenceValue) {
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        ::WaitForSingleObject(fenceEvent, INFINITE);
    }
}
