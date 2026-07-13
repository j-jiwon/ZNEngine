#pragma once
#include "PostProcessPass.h"
#include "../RenderTexture.h"
#include "../SwapChain.h"
#include "../ZNUtils.h"
#include "Graphics/ZNShader.h"
#include <d3d12.h>

namespace ZNFramework {

// Final pass of the HDR pipeline: adds the bloom mip chain onto the lit HDR
// SceneColor target, applies ACES filmic tone mapping + gamma correction, and
// writes the LDR back buffer.
class ToneMappingPass : public PostProcessPass {
public:
    ToneMappingPass(RenderTexture* sceneColor, RenderTexture* bloom,
                    SwapChain* swapChain, ZNShader* toneMapShader);

    void SetBloomIntensity(float intensity) { bloomIntensity = intensity; }

protected:
    void Draw(ID3D12GraphicsCommandList* cmd, RenderGraph& rg,
             const std::vector<RGResource*>& inputs, RGResource* output) override;

private:
    void CreateFullscreenQuad();
    void CreateConstantBuffer();
    void CreateDescriptorHeap();

    RenderTexture* sceneColor;
    RenderTexture* bloom;
    SwapChain*     swapChain;
    ZNShader*      toneMapShader;

    float bloomIntensity = 1.0f;

    ComPtr<ID3D12Resource>    quadVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW  quadVertexBufferView = {};

    ComPtr<ID3D12Resource> constantBuffer;
    void*                  mappedConstantBuffer = nullptr;

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
};

} // namespace ZNFramework
