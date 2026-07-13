#pragma once
#include "ZNUtils.h"
#include "CubeRenderTexture.h"

namespace ZNFramework
{
    class Shader;
    class ZNCamera;

    // Draws the visible skybox. Two draw modes, sharing a quad but each with its own
    // shader/PSO (different destination RTV formats) and descriptor heap(s):
    //  - DrawResolve(): main camera, once per frame, into HDR SceneColor. Depth-tested
    //    (skips pixels where real geometry exists) via skybox_resolve.hlsli.
    //  - DrawBackground(): one call per cube-capture face, before scene geometry, into
    //    the LDR capture cube (see ZNScene::AddCubemapCapture) via skybox_background.hlsli.
    //    Unconditional — always fills the whole face; subsequent depth-tested geometry
    //    draws naturally overwrite it where objects exist.
    // Both fall back to a 1x1 black cube when no scene has registered a skybox, so
    // callers never need to branch on whether one is active.
    class SkyboxRenderer
    {
    public:
        void Init();

        void DrawResolve(ID3D12GraphicsCommandList* cmd, ZNCamera* camera,
                         D3D12_CPU_DESCRIPTOR_HANDLE depthSRV,
                         bool hasSkybox, D3D12_CPU_DESCRIPTOR_HANDLE skyboxSRV,
                         D3D12_CPU_DESCRIPTOR_HANDLE dstRTV, uint32 width, uint32 height);

        // Also re-binds the shared root signature + table descriptor heap afterward (this
        // temporarily switches to its own dedicated heap to draw), since the caller's
        // per-object rendering that follows expects the shared heap already bound —
        // mirroring ForwardRenderPass/OffscreenCameraPass's own restore-after-use pattern.
        void DrawBackground(ID3D12GraphicsCommandList* cmd, uint32 faceIndex,
                            bool hasSkybox, D3D12_CPU_DESCRIPTOR_HANDLE skyboxSRV,
                            D3D12_CPU_DESCRIPTOR_HANDLE dstRTV, uint32 faceSize);

    private:
        void CreateFullscreenQuad();
        void CreateShaders();
        void CreateConstantBuffers();
        void CreateDescriptorHeaps();
        void CreateFallbackCube();

        // Matches skybox_resolve.hlsli's cbSkybox layout exactly.
        struct ResolveCB
        {
            float forward[3]; float tanHalfFovX;
            float right[3];   float tanHalfFovY;
            float up[3];      float _pad;
        };

        // Matches skybox_background.hlsli's cbSkybox layout exactly.
        struct BackgroundCB
        {
            float forward[3]; float _pad0;
            float right[3];   float _pad1;
            float up[3];      float _pad2;
        };

        Shader* resolveShader    = nullptr; // R16G16B16A16_FLOAT (SceneColor)
        Shader* backgroundShader = nullptr; // R8G8B8A8_UNORM (cube capture)

        ComPtr<ID3D12Resource>   quadVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW quadVertexBufferView = {};

        // Resolve: one CB/heap, reused every frame — only ever one resolve draw per frame.
        ComPtr<ID3D12Resource>       resolveCB;
        void*                        mappedResolveCB = nullptr;
        ComPtr<ID3D12DescriptorHeap> resolveHeap;

        // Background: one CB/heap per face. All six draws happen within the same
        // recorded command list before any of it runs on the GPU, so a shared CB/heap
        // would corrupt earlier faces once the CPU overwrites it for a later one (same
        // reasoning as BloomChain/IBLBaker's per-draw heaps).
        ComPtr<ID3D12Resource>       backgroundCB[6];
        void*                        mappedBackgroundCB[6] = {};
        ComPtr<ID3D12DescriptorHeap> backgroundHeaps[6];

        CubeRenderTexture fallbackCube; // 1x1 black, used when no skybox is active
    };
}
