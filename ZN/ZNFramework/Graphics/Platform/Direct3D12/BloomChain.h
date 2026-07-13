#pragma once
#include "ZNUtils.h"
#include "RenderTexture.h"

namespace ZNFramework
{
    class Shader;

    // Bright-pass + mip downsample/upsample bloom chain, operating on the HDR
    // SceneColor target. mips[0] is full resolution (bright-pass extraction target
    // and final combined result); mips[1..N-1] are each half the previous resolution.
    // Render() runs: threshold(SceneColor -> mip0) -> downsample chain (mip[i] ->
    // mip[i+1]) -> upsample chain, additively combining back down to mip0, which is
    // the final bloom result sampled by ToneMappingPass.
    class BloomChain
    {
    public:
        void Init(uint32 width, uint32 height);
        void Resize(uint32 width, uint32 height);
        void Render(RenderTexture* sceneColor, ID3D12GraphicsCommandList* cmd);

        RenderTexture* GetOutputRT() { return &mips[0]; }
        ID3D12Resource* GetOutputResource() const { return mips[0].GetResource(); }

        void SetThreshold(float t) { threshold = t; }
        float GetThreshold() const { return threshold; }

    private:
        void CreateResources(uint32 width, uint32 height);
        void CreateShaders();
        void CreateFullscreenQuad();
        void CreateConstantBuffer();
        void CreateDescriptorHeaps();

        void DrawFullscreen(ID3D12GraphicsCommandList* cmd, Shader* shader,
                            D3D12_CPU_DESCRIPTOR_HANDLE srcSRV,
                            RenderTexture& dst, ID3D12DescriptorHeap* heap,
                            ID3D12Resource* cb = nullptr);

        // Transitions mips[index] if it isn't already in newState. Private mirror of
        // RenderGraph::Transition — these mips are never registered with the RenderGraph
        // (only mips[0], exposed as the "Bloom" resource, is; see GetOutputResource()).
        void TransitionMip(ID3D12GraphicsCommandList* cmd, uint32 index, D3D12_RESOURCE_STATES newState);

        static constexpr uint32 kNumMips = 5;

        RenderTexture mips[kNumMips]; // 0 = full res ... kNumMips-1 = smallest
        D3D12_RESOURCE_STATES mipState[kNumMips];

        Shader* thresholdShader  = nullptr;
        Shader* downsampleShader = nullptr;
        Shader* upsampleShader   = nullptr;

        ComPtr<ID3D12Resource>    quadVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW  quadVertexBufferView = {};

        // One dedicated descriptor heap per draw in the chain (threshold + downsamples +
        // upsamples), so each draw's single-SRV table lives in its own heap region —
        // required since the whole frame's command list is recorded before any of it
        // executes on the GPU, so reusing one heap slot across sequential draws would
        // corrupt earlier draws once the CPU overwrites it for a later one.
        ComPtr<ID3D12DescriptorHeap> thresholdHeap;
        ComPtr<ID3D12DescriptorHeap> downsampleHeaps[kNumMips - 1];
        ComPtr<ID3D12DescriptorHeap> upsampleHeaps[kNumMips - 1];

        ComPtr<ID3D12Resource> thresholdConstantBuffer;
        void*                  mappedThresholdBuffer = nullptr;

        float threshold = 1.0f;

        uint32 width  = 0;
        uint32 height = 0;
    };
}
