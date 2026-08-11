#pragma once
#include "../RenderGraph.h"
#include "../DepthStencilBuffer.h"
#include "../RenderTexture.h"
#include <d3d12.h>
#include <functional>

namespace ZNFramework {

class ForwardRenderPass : public RenderPass {
public:
    ForwardRenderPass(RenderTexture* sceneColor,
                      DepthStencilBuffer* dsBuffer,
                      ID3D12RootSignature* rootSig,
                      ID3D12DescriptorHeap* tableDescHeap,
                      bool& isForwardPassRef,
                      std::function<void()> cb)
        : RenderPass("Forward")
        , sceneColor(sceneColor), dsBuffer(dsBuffer)
        , rootSig(rootSig), tableDescHeap(tableDescHeap)
        , isForwardPassRef(isForwardPassRef)
        , renderCb(std::move(cb))
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        if (!renderCb) return;

        // Composite forward/transparent lighting into the linear HDR target so it
        // participates in bloom and tone mapping. Preserve the main depth buffer.
        RGResource* sceneColorRes = rg.GetResource("SceneColor");
        rg.Transition(cmd, sceneColorRes, D3D12_RESOURCE_STATE_RENDER_TARGET);

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneColor->GetRTV();
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsBuffer->GetDSVCpuHandle();
        cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

        // Re-bind root signature and table descriptor heap
        // (deferred lighting pass changes them for its own use)
        cmd->SetGraphicsRootSignature(rootSig);
        cmd->SetDescriptorHeaps(1, &tableDescHeap);

        isForwardPassRef = true;
        renderCb();
        isForwardPassRef = false;
    }

private:
    RenderTexture*        sceneColor;
    DepthStencilBuffer*   dsBuffer;
    ID3D12RootSignature*  rootSig;
    ID3D12DescriptorHeap* tableDescHeap;
    bool&                 isForwardPassRef;
    std::function<void()> renderCb;
};

} // namespace ZNFramework
