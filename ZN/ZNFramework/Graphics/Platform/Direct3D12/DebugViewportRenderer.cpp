#include "DebugViewportRenderer.h"
#include "GBufferManager.h"
#include "Shader.h"
#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "TableDescriptorHeap.h"
#include "ZNFramework.h"

using namespace ZNFramework;

struct DebugVertex
{
    float pos[3];
    float uv[2];
};

void DebugViewportRenderer::Init()
{
    CreateFullscreenQuad();
}

void DebugViewportRenderer::CreateFullscreenQuad()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // Create fullscreen quad vertices (NDC coordinates)
    DebugVertex vertices[] = {
        // Position (x, y, z)         UV (u, v)
        { {-1.0f,  1.0f, 0.0f},      {0.0f, 0.0f} },  // Top-left
        { { 1.0f,  1.0f, 0.0f},      {1.0f, 0.0f} },  // Top-right
        { {-1.0f, -1.0f, 0.0f},      {0.0f, 1.0f} },  // Bottom-left
        { { 1.0f, -1.0f, 0.0f},      {1.0f, 1.0f} }   // Bottom-right
    };

    uint32 bufferSize = sizeof(vertices);

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&quadVertexBuffer)
    ));

    // Copy vertex data
    void* mappedData = nullptr;
    quadVertexBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, vertices, bufferSize);
    quadVertexBuffer->Unmap(0, nullptr);

    // Create vertex buffer view
    quadVertexBufferView.BufferLocation = quadVertexBuffer->GetGPUVirtualAddress();
    quadVertexBufferView.SizeInBytes = bufferSize;
    quadVertexBufferView.StrideInBytes = sizeof(DebugVertex);
}

void DebugViewportRenderer::RenderDebugViews(GBufferManager* gbufferManager, uint32 screenWidth, uint32 screenHeight)
{
    if (!isEnabled || !gbufferManager)
        return;

    // Define viewport sizes (percentage of screen)
    float viewportWidthPercent = 0.20f;   // 20% of screen width
    float viewportHeightPercent = 0.15f;  // 15% of screen height

    float vpWidth = viewportWidthPercent;
    float vpHeight = viewportHeightPercent;

    // Render 3 debug views in top-right corner stacked vertically
    // View 0: Depth (top)
    RenderViewport(0, gbufferManager->GetDepthCopySRV(),
                   1.0f - vpWidth, 0.0f, vpWidth, vpHeight, screenWidth, screenHeight);

    // View 1: Base Color (middle)
    RenderViewport(1, gbufferManager->GetBaseColorSRV(),
                   1.0f - vpWidth, vpHeight, vpWidth, vpHeight, screenWidth, screenHeight);

    // View 2: Normal (bottom)
    RenderViewport(2, gbufferManager->GetNormalSRV(),
                   1.0f - vpWidth, vpHeight * 2.0f, vpWidth, vpHeight, screenWidth, screenHeight);
}

void DebugViewportRenderer::RenderViewport(int viewType, D3D12_CPU_DESCRIPTOR_HANDLE srv,
                                          float x, float y, float width, float height,
                                          uint32 screenWidth, uint32 screenHeight)
{
    CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    ID3D12GraphicsCommandList* cmdList = queue->CommandList();

    // Set viewport and scissor rect
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = x * screenWidth;
    viewport.TopLeftY = y * screenHeight;
    viewport.Width = width * screenWidth;
    viewport.Height = height * screenHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    D3D12_RECT scissorRect;
    scissorRect.left = static_cast<LONG>(viewport.TopLeftX);
    scissorRect.top = static_cast<LONG>(viewport.TopLeftY);
    scissorRect.right = static_cast<LONG>(viewport.TopLeftX + viewport.Width);
    scissorRect.bottom = static_cast<LONG>(viewport.TopLeftY + viewport.Height);

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    // TODO: Bind debug shader and draw fullscreen quad
    // This will be completed when we create the debug view shader

    // For now, just set up the geometry
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList->IASetVertexBuffers(0, 1, &quadVertexBufferView);

    // Draw quad (4 vertices, triangle strip)
    cmdList->DrawInstanced(4, 1, 0, 0);
}
