#pragma once
#include "PostProcessPass.h"
#include "../RenderTexture.h"
#include "../SwapChain.h"
#include "../ZNUtils.h"
#include "Graphics/ZNShader.h"
#include <d3d12.h>

namespace ZNFramework {

// Final pass of the HDR pipeline: reads the lit HDR SceneColor target, applies ACES
// filmic tone mapping + gamma correction, and writes the LDR back buffer.
class ToneMappingPass : public PostProcessPass {
public:
    ToneMappingPass(RenderTexture* sceneColor, SwapChain* swapChain, ZNShader* toneMapShader);

protected:
    void Draw(ID3D12GraphicsCommandList* cmd, RenderGraph& rg,
             RGResource* input, RGResource* output) override;

private:
    void CreateFullscreenQuad();
    void CreateDescriptorHeap();

    RenderTexture* sceneColor;
    SwapChain*     swapChain;
    ZNShader*      toneMapShader;

    ComPtr<ID3D12Resource>    quadVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW  quadVertexBufferView = {};

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
};

} // namespace ZNFramework
