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
#include "Passes/ShadowPass.h"
#include "Passes/GBufferPass.h"
#include "Passes/DeferredLightingRenderPass.h"
#include "Passes/ForwardRenderPass.h"
#include "Passes/ImGuiRenderPass.h"
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

    // Get concrete types needed by the passes
    auto* rootSig  = GraphicsContext::GetInstance().GetAs<RootSignature>();
    auto* tdh      = GraphicsContext::GetInstance().GetAs<TableDescriptorHeap>();
    auto* dsBuffer = GraphicsContext::GetInstance().GetAs<DepthStencilBuffer>();
    ZNShader* gbufShader = GraphicsContext::GetInstance().GetGBufferShader();

    // --- Shadow pass ---
    if (shadowMap) {
        renderGraph.AddPass(std::make_unique<ShadowPass>(
            shadowMap,
            [this]() { if (shadowRenderCallback) shadowRenderCallback(); }));
    }

    // --- GBuffer pass (scene geometry) ---
    if (gbufferManager) {
        renderGraph.AddPass(std::make_unique<GBufferPass>(
            gbufferManager, gbufShader, dsBuffer, swapChain,
            [this]() { if (gbufferRenderCallback) gbufferRenderCallback(); }));
    }

    // --- Deferred lighting pass ---
    if (deferredLightingPass && gbufferManager) {
        renderGraph.AddPass(std::make_unique<DeferredLightingRenderPass>(
            deferredLightingPass, gbufferManager, shadowMap, swapChain));
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
    swapChain->SwapIndex();

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
