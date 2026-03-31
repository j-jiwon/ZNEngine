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
        void SetEnabled(bool enabled) { isEnabled = enabled; }
        bool IsEnabled() const { return isEnabled; }

    private:
        void CreateFullscreenQuad();
        void RenderViewport(int viewType, D3D12_CPU_DESCRIPTOR_HANDLE srv,
                           float x, float y, float width, float height,
                           uint32 screenWidth, uint32 screenHeight);

    private:
        ComPtr<ID3D12Resource> quadVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW quadVertexBufferView = {};

        Shader* debugViewShader = nullptr;
        bool isEnabled = true;
    };
}
