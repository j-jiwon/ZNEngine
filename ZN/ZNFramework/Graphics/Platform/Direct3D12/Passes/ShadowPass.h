#pragma once
#include "../RenderGraph.h"
#include "../ShadowMap.h"
#include <functional>

namespace ZNFramework {

class ShadowPass : public RenderPass {
public:
    ShadowPass(ShadowMap* shadow, std::function<void()> cb)
        : RenderPass("Shadow"), shadowMap(shadow), renderCb(std::move(cb)) {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        if (!shadowMap || !renderCb) return;

        RGResource* res = rg.GetResource("ShadowMap");
        rg.Transition(cmd, res, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        uint32_t w = shadowMap->GetWidth(), h = shadowMap->GetHeight();
        D3D12_VIEWPORT vp   = { 0, 0, (float)w, (float)h, 0.f, 1.f };
        D3D12_RECT     rect = { 0, 0, (LONG)w,  (LONG)h };
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &rect);

        D3D12_CPU_DESCRIPTOR_HANDLE dsv = shadowMap->GetDSV();
        cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        cmd->OMSetRenderTargets(0, nullptr, FALSE, &dsv);

        renderCb();

        rg.Transition(cmd, res, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

private:
    ShadowMap*            shadowMap;
    std::function<void()> renderCb;
};

} // namespace ZNFramework
