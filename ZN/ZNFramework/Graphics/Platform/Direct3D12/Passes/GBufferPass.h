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
                std::function<void()> cb)
        : RenderPass("GBuffer")
        , gbufMgr(gbufMgr), gbufShader(gbufShader)
        , dsBuffer(dsBuffer), swapChain(swapChain)
        , renderCb(std::move(cb))
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        // Restore main viewport (shadow pass may have changed it)
        float w = (float)swapChain->Width(), h = (float)swapChain->Height();
        D3D12_VIEWPORT vp   = { 0, 0, w, h, 0.f, 1.f };
        D3D12_RECT     rect = { 0, 0, (LONG)w, (LONG)h };
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &rect);

        // Transition all GBuffer targets → RENDER_TARGET
        static const char* names[] = {
            "GBuf_BaseColor","GBuf_Normal","GBuf_DepthCopy","GBuf_WorldPos","GBuf_ARM"
        };
        for (auto n : names)
            rg.Transition(cmd, rg.GetResource(n), D3D12_RESOURCE_STATE_RENDER_TARGET);

        if (gbufShader) gbufShader->Bind();

        float black[4]  = { 0.f, 0.f, 0.f, 1.f };
        float zero[4]   = { 0.f, 0.f, 0.f, 0.f };
        float depth1[4] = { 1.f, 0.f, 0.f, 0.f };
        cmd->ClearRenderTargetView(gbufMgr->GetBaseColorRTV(), black,  0, nullptr);
        cmd->ClearRenderTargetView(gbufMgr->GetNormalRTV(),    zero,   0, nullptr);
        cmd->ClearRenderTargetView(gbufMgr->GetDepthCopyRTV(),depth1, 0, nullptr);
        cmd->ClearRenderTargetView(gbufMgr->GetWorldPosRTV(),  zero,   0, nullptr);
        cmd->ClearRenderTargetView(gbufMgr->GetARMRTV(),       zero,   0, nullptr);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[5] = {
            gbufMgr->GetBaseColorRTV(), gbufMgr->GetNormalRTV(),
            gbufMgr->GetDepthCopyRTV(), gbufMgr->GetWorldPosRTV(),
            gbufMgr->GetARMRTV()
        };
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsBuffer->GetDSVCpuHandle();
        cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        cmd->OMSetRenderTargets(5, rtvs, FALSE, &dsv);

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
    std::function<void()> renderCb;
};

} // namespace ZNFramework
