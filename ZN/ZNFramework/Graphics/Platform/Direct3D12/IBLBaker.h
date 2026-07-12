#pragma once
#include "ZNUtils.h"
#include "CubeRenderTexture.h"
#include "RenderTexture.h"

namespace ZNFramework
{
    class Shader;

    // Bakes the split-sum IBL data (diffuse irradiance + roughness-prefiltered specular
    // + BRDF LUT) from whichever env cubemap is currently active (CommandQueue's single
    // env-cubemap slot — see ZNScene::AddCubemapCapture/SetEnvCubemapTexture). Mirrors
    // DeferredLightingPass's split: this is the logic class; IBLBakePass is the thin
    // RenderGraph wrapper around it.
    class IBLBaker
    {
    public:
        void Init();

        // Pure math, no scene dependency — bakes once, unconditionally, on first call.
        void BakeBRDFLUT(ID3D12GraphicsCommandList* cmd);

        // Convolves irradiance + prefiltered specular from srcEnvCubeSRV. Bakes once,
        // the first time it's called (expected to be called with a real, non-fallback
        // source — the caller is responsible for only calling this once one is active).
        void BakeEnvironment(D3D12_CPU_DESCRIPTOR_HANDLE srcEnvCubeSRV, ID3D12GraphicsCommandList* cmd);

        D3D12_CPU_DESCRIPTOR_HANDLE GetIrradianceSRV() const
        {
            return envBaked ? irradianceCube.GetSRVCpuHandle() : fallbackIrradianceCube.GetSRVCpuHandle();
        }
        D3D12_CPU_DESCRIPTOR_HANDLE GetPrefilteredSRV() const
        {
            return envBaked ? prefilteredCube.GetSRVCpuHandle() : fallbackPrefilteredCube.GetSRVCpuHandle();
        }
        D3D12_CPU_DESCRIPTOR_HANDLE GetBRDFLUTSRV() const { return brdfLUT.GetSRVCpuHandle(); }

        uint32 GetPrefilteredMipCount() const { return kPrefilterMips; }

    private:
        static constexpr uint32 kIrradianceSize    = 32;
        static constexpr uint32 kPrefilterSize     = 128;
        static constexpr uint32 kPrefilterMips     = 5;
        static constexpr uint32 kBRDFLUTSize       = 256;
        static constexpr uint32 kNumPrefilterDraws = 6 * kPrefilterMips;

        // Matches the cbFace layout in ibl_irradiance.hlsli / ibl_prefilter.hlsli exactly
        // (irradiance leaves `roughness` unread).
        struct FaceCB
        {
            float forward[3]; float roughness;
            float right[3];   float _pad1;
            float up[3];      float _pad2;
        };

        void CreateShaders();
        void CreateFullscreenQuad();
        void CreateFallbackCubes();

        void DrawFace(ID3D12GraphicsCommandList* cmd, Shader* shader,
                     D3D12_CPU_DESCRIPTOR_HANDLE srcSRV, D3D12_CPU_DESCRIPTOR_HANDLE dstRTV,
                     uint32 faceSize, const FaceCB& cbData,
                     ID3D12Resource* cbResource, void* mappedCB, ID3D12DescriptorHeap* heap);
        void Draw2D(ID3D12GraphicsCommandList* cmd, Shader* shader,
                   RenderTexture& dst, ID3D12DescriptorHeap* heap);

        CubeRenderTexture irradianceCube;
        CubeRenderTexture prefilteredCube;
        RenderTexture     brdfLUT;

        // 1x1 black — used until BakeEnvironment() has actually run once.
        CubeRenderTexture fallbackIrradianceCube;
        CubeRenderTexture fallbackPrefilteredCube;

        Shader* irradianceShader = nullptr;
        Shader* prefilterShader  = nullptr;
        Shader* brdfLUTShader    = nullptr;

        ComPtr<ID3D12Resource>   quadVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW quadVertexBufferView = {};

        // One CB + one dedicated descriptor heap per draw (6 irradiance faces, 6*kPrefilterMips
        // prefilter face/mip draws, 1 BRDF LUT draw) — the whole frame is recorded before any
        // of it runs on the GPU, so heap-slot/CB reuse across sequential draws would corrupt
        // earlier draws once the CPU overwrites them for a later one (same reasoning as
        // BloomChain's per-draw heaps).
        ComPtr<ID3D12Resource>       irradianceCB[6];
        void*                        mappedIrradianceCB[6] = {};
        ComPtr<ID3D12DescriptorHeap> irradianceHeaps[6];

        ComPtr<ID3D12Resource>       prefilterCB[kNumPrefilterDraws];
        void*                        mappedPrefilterCB[kNumPrefilterDraws] = {};
        ComPtr<ID3D12DescriptorHeap> prefilterHeaps[kNumPrefilterDraws];

        ComPtr<ID3D12DescriptorHeap> brdfLUTHeap;

        bool brdfBaked = false;
        bool envBaked  = false;
    };
}
