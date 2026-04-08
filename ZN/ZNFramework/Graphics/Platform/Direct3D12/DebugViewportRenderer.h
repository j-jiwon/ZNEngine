#pragma once
#include "ZNUtils.h"

namespace ZNFramework
{
    class GBufferManager;
    class Shader;

    class DebugViewportRenderer
    {
    public:
        void Init();
        void RenderDebugViews(GBufferManager* gbufferManager, uint32 screenWidth, uint32 screenHeight);
        void RenderMainView(GBufferManager* gbufferManager, uint32 screenWidth, uint32 screenHeight);
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
        Shader* lightingShader = nullptr;

        // Constant buffer for viewType
        ComPtr<ID3D12Resource> viewTypeConstantBuffer;
        void* mappedConstantBuffer = nullptr;

        // Constant buffer for lighting
        ComPtr<ID3D12Resource> lightingConstantBuffer;
        void* mappedLightingBuffer = nullptr;

        // Descriptor heap for CBV + SRV (for root signature compatibility)
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        uint32 descriptorSize = 0;
        uint32 constantBufferSize = 0; // Size per viewport (256 bytes)

        // Descriptor heap for lighting pass (kept alive across frames)
        ComPtr<ID3D12DescriptorHeap> lightingDescriptorHeap;

        bool isEnabled = true;
    };
}
