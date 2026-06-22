#pragma once
#include "../RenderGraph.h"
#include "../DepthStencilBuffer.h"
#include "../SwapChain.h"
#include <d3d12.h>
#include <functional>

namespace ZNFramework {

class ForwardRenderPass : public RenderPass {
public:
    ForwardRenderPass(SwapChain* swapChain,
                      DepthStencilBuffer* dsBuffer,
                      ID3D12RootSignature* rootSig,
                      ID3D12DescriptorHeap* tableDescHeap,
                      bool& isForwardPassRef,
                      std::function<void()> cb)
        : RenderPass("Forward")
        , swapChain(swapChain), dsBuffer(dsBuffer)
        , rootSig(rootSig), tableDescHeap(tableDescHeap)
        , isForwardPassRef(isForwardPassRef)
        , renderCb(std::move(cb))
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        if (!renderCb) return;

        // Re-bind back buffer + depth stencil (deferred lighting left no DSV)
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChain->GetBackRTV();
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
    SwapChain*            swapChain;
    DepthStencilBuffer*   dsBuffer;
    ID3D12RootSignature*  rootSig;
    ID3D12DescriptorHeap* tableDescHeap;
    bool&                 isForwardPassRef;
    std::function<void()> renderCb;
};

} // namespace ZNFramework
