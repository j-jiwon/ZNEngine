#pragma once
#include "../RenderGraph.h"
#include "../CubeRenderTexture.h"
#include "Graphics/ZNGraphicsContext.h"
#include "ZNFramework/ZNCamera.h"
#include <d3d12.h>
#include <functional>
#include <string>
#include <vector>

namespace ZNFramework {

// Renders a scene's gameObjects into all 6 faces of a CubeRenderTexture exactly once
// (on the first frame it executes), then no-ops forever after. Static bake: fine for a
// room that doesn't move; re-capture support can be added later if something dynamic
// needs to show up in the reflection.
class CubeCapturePass : public RenderPass {
public:
    CubeCapturePass(const std::string& name,
                    std::vector<ZNCamera*> cams,
                    CubeRenderTexture* output,
                    ID3D12RootSignature* rootSig,
                    ID3D12DescriptorHeap* tableDescHeap,
                    bool& isForwardPassRef,
                    std::function<void()> renderCb)
        : RenderPass(name)
        , cams(std::move(cams)), output(output)
        , rootSig(rootSig), tableDescHeap(tableDescHeap)
        , isForwardPassRef(isForwardPassRef)
        , renderCb(std::move(renderCb))
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        if (captured) return; // one-shot; resource already sits in PIXEL_SHADER_RESOURCE state

        rg.Transition(cmd, rg.GetResource(GetName()), D3D12_RESOURCE_STATE_RENDER_TARGET);

        float size = static_cast<float>(output->GetSize());
        D3D12_VIEWPORT vp   = { 0, 0, size, size, 0.f, 1.f };
        D3D12_RECT     rect = { 0, 0, static_cast<LONG>(size), static_cast<LONG>(size) };
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &rect);

        cmd->SetGraphicsRootSignature(rootSig);
        cmd->SetDescriptorHeaps(1, &tableDescHeap);

        D3D12_CPU_DESCRIPTOR_HANDLE dsv = output->GetDSV();
        float black[4] = { 0.f, 0.f, 0.f, 1.f };

        ZNCamera* prevCamera = GraphicsContext::GetInstance().GetCamera();

        for (uint32 face = 0; face < 6 && face < cams.size(); ++face)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = output->GetRTV(face);
            cmd->ClearRenderTargetView(rtv, black, 0, nullptr);
            cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
            cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

            GraphicsContext::GetInstance().SetCamera(cams[face]);

            isForwardPassRef = true;
            if (renderCb) renderCb();
            isForwardPassRef = false;
        }

        GraphicsContext::GetInstance().SetCamera(prevCamera);

        rg.Transition(cmd, rg.GetResource(GetName()), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        captured = true;
    }

private:
    std::vector<ZNCamera*> cams;
    CubeRenderTexture*     output;
    ID3D12RootSignature*   rootSig;
    ID3D12DescriptorHeap*  tableDescHeap;
    bool&                  isForwardPassRef;
    std::function<void()>  renderCb;
    bool                   captured = false;
};

} // namespace ZNFramework
