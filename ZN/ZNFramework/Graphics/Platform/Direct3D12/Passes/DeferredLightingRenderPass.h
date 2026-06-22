#pragma once
#include "../RenderGraph.h"
#include "../DeferredLightingPass.h"
#include "../GBufferManager.h"
#include "../ShadowMap.h"
#include "../SwapChain.h"

namespace ZNFramework {

class DeferredLightingRenderPass : public RenderPass {
public:
    DeferredLightingRenderPass(DeferredLightingPass* lightingPass,
                               GBufferManager* gbufMgr,
                               ShadowMap* shadowMap,
                               SwapChain* swapChain)
        : RenderPass("DeferredLighting")
        , lightingPass(lightingPass), gbufMgr(gbufMgr)
        , shadowMap(shadowMap), swapChain(swapChain)
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        // Transition back buffer → RENDER_TARGET
        RGResource* backBuf = rg.GetResource("BackBuffer");
        rg.Transition(cmd, backBuf, D3D12_RESOURCE_STATE_RENDER_TARGET);

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChain->GetBackRTV();
        float black[4] = { 0.f, 0.f, 0.f, 1.f };
        cmd->ClearRenderTargetView(rtv, black, 0, nullptr);
        cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        uint32_t w = swapChain->Width(), h = swapChain->Height();
        D3D12_VIEWPORT vp   = { 0, 0, (float)w, (float)h, 0.f, 1.f };
        D3D12_RECT     rect = { 0, 0, (LONG)w,  (LONG)h };
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &rect);

        if (lightingPass)
            lightingPass->Render(gbufMgr, shadowMap, w, h);
    }

private:
    DeferredLightingPass* lightingPass;
    GBufferManager*       gbufMgr;
    ShadowMap*            shadowMap;
    SwapChain*            swapChain;
};

} // namespace ZNFramework
