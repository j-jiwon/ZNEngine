#pragma once
#include "ZNUtils.h"

namespace ZNFramework
{
    class GBufferManager;
    class Shader;

    class DeferredLightingPass
    {
    public:
        void Init();
        void Render(GBufferManager* gbufferManager, uint32 screenWidth, uint32 screenHeight);

    private:
        void CreateFullscreenQuad();

    private:
        ComPtr<ID3D12Resource> quadVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW quadVertexBufferView = {};

        Shader* lightingShader = nullptr;

        // Constant buffer for lighting
        ComPtr<ID3D12Resource> lightingConstantBuffer;
        void* mappedLightingBuffer = nullptr;

        // Descriptor heap for lighting pass
        ComPtr<ID3D12DescriptorHeap> lightingDescriptorHeap;
    };
}
