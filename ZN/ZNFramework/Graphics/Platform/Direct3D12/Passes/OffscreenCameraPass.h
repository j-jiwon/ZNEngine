#pragma once
#include "../RenderGraph.h"
#include "../RenderTexture.h"
#include "Graphics/ZNGraphicsContext.h"
#include "ZNFramework/ZNCamera.h"
#include <d3d12.h>
#include <functional>
#include <string>

namespace ZNFramework {

class OffscreenCameraPass : public RenderPass {
public:
    OffscreenCameraPass(const std::string& name,
                        ZNCamera* camera,
                        RenderTexture* output,
                        ID3D12RootSignature* rootSig,
                        ID3D12DescriptorHeap* tableDescHeap,
                        bool& isForwardPassRef,
                        std::function<void()> renderCb)
        : RenderPass(name)
        , camera(camera), output(output)
        , rootSig(rootSig), tableDescHeap(tableDescHeap)
        , isForwardPassRef(isForwardPassRef)
        , renderCb(std::move(renderCb))
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        // Transition color RT → RENDER_TARGET (from PIXEL_SHADER_RESOURCE on subsequent frames)
        rg.Transition(cmd, rg.GetResource(GetName()), D3D12_RESOURCE_STATE_RENDER_TARGET);

        float w = static_cast<float>(output->GetWidth());
        float h = static_cast<float>(output->GetHeight());
        D3D12_VIEWPORT vp   = { 0, 0, w, h, 0.f, 1.f };
        D3D12_RECT     rect = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &rect);

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = output->GetRTV();
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = output->GetDSV();

        float black[4] = { 0.f, 0.f, 0.f, 1.f };
        cmd->ClearRenderTargetView(rtv, black, 0, nullptr);
        cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

        // Re-bind root signature and table descriptor heap
        cmd->SetGraphicsRootSignature(rootSig);
        cmd->SetDescriptorHeaps(1, &tableDescHeap);

        // Swap in the offscreen camera so Mesh::Render() picks up correct view/projection
        ZNCamera* prevCamera = GraphicsContext::GetInstance().GetCamera();
        GraphicsContext::GetInstance().SetCamera(camera);

        // isForwardPass = true so Material::Bind() calls shader->Bind() (not skipped by MRT check)
        isForwardPassRef = true;
        if (renderCb) renderCb();
        isForwardPassRef = false;

        // Restore main camera
        GraphicsContext::GetInstance().SetCamera(prevCamera);

        // Transition → PIXEL_SHADER_RESOURCE so it can be sampled in the main GBuffer pass
        rg.Transition(cmd, rg.GetResource(GetName()), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

private:
    ZNCamera*             camera;
    RenderTexture*        output;
    ID3D12RootSignature*  rootSig;
    ID3D12DescriptorHeap* tableDescHeap;
    bool&                 isForwardPassRef;
    std::function<void()> renderCb;
};

} // namespace ZNFramework
