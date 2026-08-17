#pragma once
#include "../RenderGraph.h"
#include "../GBufferManager.h"
#include "../DepthStencilBuffer.h"
#include "../SwapChain.h"
#include "Graphics/ZNShader.h"
#include <functional>

namespace ZNFramework {

class GBufferPass : public RenderPass {
public:
    GBufferPass(GBufferManager* gbufMgr, ZNShader* gbufShader,
                DepthStencilBuffer* dsBuffer, SwapChain* swapChain,
                ID3D12RootSignature* rootSig, ID3D12DescriptorHeap* tableDescHeap,
                std::function<void()> cb)
        : RenderPass("GBuffer")
        , gbufMgr(gbufMgr), gbufShader(gbufShader)
        , dsBuffer(dsBuffer), swapChain(swapChain)
        , rootSig(rootSig), tableDescHeap(tableDescHeap)
        , renderCb(std::move(cb))
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        // Restore main viewport (shadow pass may have changed it)
        float w = (float)swapChain->Width(), h = (float)swapChain->Height();
        D3D12_VIEWPORT vp   = { 0, 0, w, h, 0.f, 1.f };
        D3D12_RECT     rect = { 0, 0, (LONG)w, (LONG)h };
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &rect);

        // Re-bind the shared root signature + descriptor heap. The one-shot IBLBakePass /
        // CubeCapturePass that run just before us leave THEIR own shader-visible heap bound, so
        // without this the per-object CommitTable() here would set a root table with a shared-heap
        // handle while a different heap is current — the GPU then reads garbage descriptors for
        // the whole GBuffer on that frame (same "restore" convention as Forward/Offscreen passes).
        cmd->SetGraphicsRootSignature(rootSig);
        cmd->SetDescriptorHeaps(1, &tableDescHeap);

        // Transition all GBuffer targets → RENDER_TARGET
        static const char* names[] = {
            "GBuf_BaseColor","GBuf_Normal","GBuf_DepthCopy","GBuf_WorldPos","GBuf_ARM",
            "GBuf_Emissive"
        };
        for (auto n : names)
            rg.Transition(cmd, rg.GetResource(n), D3D12_RESOURCE_STATE_RENDER_TARGET);

        if (gbufShader) gbufShader->Bind();

        // Clear values come from GBufferManager so they match the resources' optimised clear
        // values (see GBufferManager::ClearColor) and take the fast clear path.
        cmd->ClearRenderTargetView(gbufMgr->GetBaseColorRTV(), GBufferManager::ClearColor(0), 0, nullptr);
        cmd->ClearRenderTargetView(gbufMgr->GetNormalRTV(),    GBufferManager::ClearColor(1), 0, nullptr);
        cmd->ClearRenderTargetView(gbufMgr->GetDepthCopyRTV(), GBufferManager::ClearColor(2), 0, nullptr);
        cmd->ClearRenderTargetView(gbufMgr->GetWorldPosRTV(),  GBufferManager::ClearColor(3), 0, nullptr);
        cmd->ClearRenderTargetView(gbufMgr->GetARMRTV(),       GBufferManager::ClearColor(4), 0, nullptr);
        cmd->ClearRenderTargetView(gbufMgr->GetEmissiveRTV(),  GBufferManager::ClearColor(5), 0, nullptr);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[6] = {
            gbufMgr->GetBaseColorRTV(), gbufMgr->GetNormalRTV(),
            gbufMgr->GetDepthCopyRTV(), gbufMgr->GetWorldPosRTV(),
            gbufMgr->GetARMRTV(),       gbufMgr->GetEmissiveRTV()
        };
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsBuffer->GetDSVCpuHandle();
        cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        cmd->OMSetRenderTargets(_countof(rtvs), rtvs, FALSE, &dsv);

        if (renderCb) renderCb();

        // Transition all GBuffer targets → PIXEL_SHADER_RESOURCE for lighting pass
        for (auto n : names)
            rg.Transition(cmd, rg.GetResource(n), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

private:
    GBufferManager*       gbufMgr;
    ZNShader*             gbufShader;
    DepthStencilBuffer*   dsBuffer;
    SwapChain*            swapChain;
    ID3D12RootSignature*  rootSig;
    ID3D12DescriptorHeap* tableDescHeap;
    std::function<void()> renderCb;
};

} // namespace ZNFramework
