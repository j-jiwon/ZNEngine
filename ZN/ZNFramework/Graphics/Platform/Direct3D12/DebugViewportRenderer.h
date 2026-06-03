#pragma once
#include "ZNUtils.h"

namespace ZNFramework
{
    class GBufferManager;
    class ShadowMap;
    class Shader;

    class DebugViewportRenderer
    {
    public:
        void Init();
        void RenderDebugViews(GBufferManager* gbufferManager, ShadowMap* shadowMap, uint32 screenWidth, uint32 screenHeight);
        void SetEnabled(bool enabled) { isEnabled = enabled; }
        bool IsEnabled() const { return isEnabled; }

    private:
        void CreateFullscreenQuad();
        void UpdateViewTypeConstants(int viewportIndex, int viewType);
        void RenderViewport(int viewportIndex, D3D12_CPU_DESCRIPTOR_HANDLE srv,
                           float x, float y, float width, float height,
                           uint32 screenWidth, uint32 screenHeight);

    private:
        ComPtr<ID3D12Resource> quadVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW quadVertexBufferView = {};

        Shader* debugViewShader = nullptr;

        // Constant buffer for viewType
        ComPtr<ID3D12Resource> viewTypeConstantBuffer;
        void* mappedConstantBuffer = nullptr;

        // Descriptor heap for CBV + SRV
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        uint32 descriptorSize = 0;
        uint32 constantBufferSize = 0;

        bool isEnabled = true;
    };
}
