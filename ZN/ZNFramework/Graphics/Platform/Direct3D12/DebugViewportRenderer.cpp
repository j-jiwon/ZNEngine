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
    float pos[3];      // POSITION (offset 0)
    float color[4];    // COLOR (offset 12) - unused but needed for input layout
    float uv[2];       // TEXCOORD (offset 28)
    float normal[3];   // NORMAL (offset 36) - unused but needed for input layout
};

void DebugViewportRenderer::Init()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    CreateFullscreenQuad();

    // Load debug view shader
    debugViewShader = new Shader();
    std::filesystem::path shaderPath = GetResourcePath() / L"Shaders" / L"debug_view.hlsli";
    debugViewShader->Load(shaderPath);
    debugViewShader->DisableDepthTest(); // No depth testing for debug viewports

    // Create constant buffer for viewType (16 bytes aligned)
    // Need 4 separate 256-byte regions for 4 viewports
    constantBufferSize = (sizeof(int) + 255) & ~255; // Align to 256 bytes (per viewport)
    uint32 totalBufferSize = constantBufferSize * 4; // 4 viewports
    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBufferSize);

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&viewTypeConstantBuffer)
    ));

    // Map constant buffer (keep it mapped)
    viewTypeConstantBuffer->Map(0, nullptr, &mappedConstantBuffer);

    // Create descriptor heap for multiple viewports
    // Each viewport needs: CBV (b0~b4: 5) + SRV (t0~t4: 5) = 10 descriptors
    // We need 4 viewports (1 main + 3 debug) = 40 descriptors total
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 40; // 4 viewports * 10 descriptors per viewport
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)));
    descriptorSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create CBV for viewType constant buffer at b0 for each viewport
    // Each viewport gets its own 256-byte region in the constant buffer
    for (uint32 i = 0; i < 4; ++i)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = viewTypeConstantBuffer->GetGPUVirtualAddress() + (i * constantBufferSize);
        cbvDesc.SizeInBytes = constantBufferSize;

        D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        cbvHandle.ptr += descriptorSize * (i * 10); // Skip to start of each viewport's descriptor set
        device->Device()->CreateConstantBufferView(&cbvDesc, cbvHandle);
    }
}

void DebugViewportRenderer::CreateFullscreenQuad()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // Create fullscreen quad vertices (NDC coordinates)
    DebugVertex vertices[] = {
        // Position (x, y, z)         Color (r,g,b,a)          UV (u, v)         Normal (x,y,z)
        { {-1.0f,  1.0f, 0.0f},      {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f},    {0.0f, 0.0f, 1.0f} },  // Top-left
        { { 1.0f,  1.0f, 0.0f},      {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f},    {0.0f, 0.0f, 1.0f} },  // Top-right
        { {-1.0f, -1.0f, 0.0f},      {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f},    {0.0f, 0.0f, 1.0f} },  // Bottom-left
        { { 1.0f, -1.0f, 0.0f},      {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f},    {0.0f, 0.0f, 1.0f} }   // Bottom-right
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

void DebugViewportRenderer::UpdateViewTypeConstants(int viewportIndex, int viewType)
{
    struct ViewTypeData {
        int viewType;
        float padding[3];
    };
    ViewTypeData data = { viewType, {0.0f, 0.0f, 0.0f} };
    uint32 constantBufferOffset = viewportIndex * constantBufferSize;
    void* targetAddress = static_cast<uint8*>(mappedConstantBuffer) + constantBufferOffset;
    memcpy(targetAddress, &data, sizeof(ViewTypeData));
}

void DebugViewportRenderer::RenderMainView(D3D12_CPU_DESCRIPTOR_HANDLE srv, uint32 screenWidth, uint32 screenHeight)
{
    // Update constant buffer for viewport 0 (BaseColor)
    UpdateViewTypeConstants(0, 1);

    // Render G-Buffer BaseColor to fullscreen
    RenderViewport(0, srv, 0.0f, 0.0f, 1.0f, 1.0f, screenWidth, screenHeight);
}

void DebugViewportRenderer::RenderDebugViews(GBufferManager* gbufferManager, uint32 screenWidth, uint32 screenHeight)
{
    if (!isEnabled || !gbufferManager)
        return;

    // Update all constant buffers BEFORE any draw calls
    UpdateViewTypeConstants(1, 0); // Depth
    UpdateViewTypeConstants(2, 1); // BaseColor
    UpdateViewTypeConstants(3, 2); // Normal

    // Define viewport sizes (percentage of screen)
    float viewportWidthPercent = 0.20f;   // 20% of screen width
    float viewportHeightPercent = 0.15f;  // 15% of screen height

    float vpWidth = viewportWidthPercent;
    float vpHeight = viewportHeightPercent;

    // Render 3 debug views in top-right corner stacked vertically
    // viewportIndex 1: Depth (top)
    RenderViewport(1, gbufferManager->GetDepthCopySRV(),
                   1.0f - vpWidth, 0.0f, vpWidth, vpHeight, screenWidth, screenHeight);

    // viewportIndex 2: Base Color (middle)
    RenderViewport(2, gbufferManager->GetBaseColorSRV(),
                   1.0f - vpWidth, vpHeight, vpWidth, vpHeight, screenWidth, screenHeight);

    // viewportIndex 3: Normal (bottom)
    RenderViewport(3, gbufferManager->GetNormalSRV(),
                   1.0f - vpWidth, vpHeight * 2.0f, vpWidth, vpHeight, screenWidth, screenHeight);
}

void DebugViewportRenderer::RenderViewport(int viewportIndex, D3D12_CPU_DESCRIPTOR_HANDLE srv,
                                          float x, float y, float width, float height,
                                          uint32 screenWidth, uint32 screenHeight)
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    ID3D12GraphicsCommandList* cmdList = queue->CommandList();

    // Calculate descriptor offset for this viewport (each viewport uses 10 descriptors)
    uint32 descriptorSetOffset = viewportIndex * 10;

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

    // Copy G-Buffer SRV to this viewport's descriptor set at t0 position (offset + 5)
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    destHandle.ptr += descriptorSize * (descriptorSetOffset + 5); // Skip to this viewport's t0 slot
    device->Device()->CopyDescriptorsSimple(1, destHandle, srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Bind debug shader
    if (debugViewShader)
    {
        debugViewShader->Bind();
    }

    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { descriptorHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // Set root descriptor table pointing to this viewport's descriptor set
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += descriptorSize * descriptorSetOffset; // Skip to this viewport's descriptor set
    cmdList->SetGraphicsRootDescriptorTable(0, gpuHandle);

    // Set geometry
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList->IASetVertexBuffers(0, 1, &quadVertexBufferView);

    // Draw quad (4 vertices, triangle strip)
    cmdList->DrawInstanced(4, 1, 0, 0);
}
