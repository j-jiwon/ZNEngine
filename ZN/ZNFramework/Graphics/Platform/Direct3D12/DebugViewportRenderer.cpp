#include "DebugViewportRenderer.h"
#include "GBufferManager.h"
#include "ShadowMap.h"
#include "Shader.h"
#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "ZNFramework.h"

using namespace ZNFramework;

struct DebugVertex
{
    float pos[3];
    float color[4];
    float uv[2];
    float normal[3];
};

void DebugViewportRenderer::Init()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    CreateFullscreenQuad();

    // Load debug view shader
    debugViewShader = new Shader();
    std::filesystem::path shaderPath = GetResourcePath() / L"Shaders" / L"debug_view.hlsli";
    debugViewShader->Load(shaderPath);
    debugViewShader->DisableDepthTest();

    // Create constant buffer for viewType
    constantBufferSize = (sizeof(int) + 255) & ~255;
    uint32 totalBufferSize = constantBufferSize * 5; // 5 viewports (Depth, BaseColor, Normal, ShadowMap + 1 spare)
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

    viewTypeConstantBuffer->Map(0, nullptr, &mappedConstantBuffer);

    // Create descriptor heap for multiple viewports
    // Each viewport needs: CBV (b0~b4: 5) + SRV (t0~t4: 5) = 10 descriptors
    // We need 5 viewports = 50 descriptors total
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 50;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)));
    descriptorSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create CBV for viewType constant buffer at b0 for each viewport
    for (uint32 i = 0; i < 5; ++i)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = viewTypeConstantBuffer->GetGPUVirtualAddress() + (i * constantBufferSize);
        cbvDesc.SizeInBytes = constantBufferSize;

        D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        cbvHandle.ptr += descriptorSize * (i * 10);
        device->Device()->CreateConstantBufferView(&cbvDesc, cbvHandle);
    }
}

void DebugViewportRenderer::CreateFullscreenQuad()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    DebugVertex vertices[] = {
        { {-1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { { 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { {-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
        { { 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }
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

    void* mappedData = nullptr;
    quadVertexBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, vertices, bufferSize);
    quadVertexBuffer->Unmap(0, nullptr);

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

void DebugViewportRenderer::RenderDebugViews(GBufferManager* gbufferManager, ShadowMap* shadowMap, uint32 screenWidth, uint32 screenHeight)
{
    if (!isEnabled || !gbufferManager)
        return;

    // Update all constant buffers BEFORE any draw calls
    UpdateViewTypeConstants(1, 0); // Depth
    UpdateViewTypeConstants(2, 1); // BaseColor
    UpdateViewTypeConstants(3, 2); // Normal
    UpdateViewTypeConstants(4, 3); // ShadowMap

    // Define viewport sizes (percentage of screen)
    float viewportWidthPercent = 0.20f;
    float viewportHeightPercent = 0.15f;

    float vpWidth = viewportWidthPercent;
    float vpHeight = viewportHeightPercent;

    // Render 4 debug views in top-right corner stacked vertically
    RenderViewport(1, gbufferManager->GetDepthCopySRV(),
                   1.0f - vpWidth, 0.0f, vpWidth, vpHeight, screenWidth, screenHeight);

    RenderViewport(2, gbufferManager->GetBaseColorSRV(),
                   1.0f - vpWidth, vpHeight, vpWidth, vpHeight, screenWidth, screenHeight);

    RenderViewport(3, gbufferManager->GetNormalSRV(),
                   1.0f - vpWidth, vpHeight * 2.0f, vpWidth, vpHeight, screenWidth, screenHeight);

    // Render shadow map if available
    if (shadowMap)
    {
        RenderViewport(4, shadowMap->GetSRV(),
                       1.0f - vpWidth, vpHeight * 3.0f, vpWidth, vpHeight, screenWidth, screenHeight);
    }
}

void DebugViewportRenderer::RenderViewport(int viewportIndex, D3D12_CPU_DESCRIPTOR_HANDLE srv,
                                          float x, float y, float width, float height,
                                          uint32 screenWidth, uint32 screenHeight)
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    ID3D12GraphicsCommandList* cmdList = queue->CommandList();

    uint32 descriptorSetOffset = viewportIndex * 10;

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

    // Copy G-Buffer SRV to this viewport's descriptor set at t0 position
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    destHandle.ptr += descriptorSize * (descriptorSetOffset + 5);
    device->Device()->CopyDescriptorsSimple(1, destHandle, srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    if (debugViewShader)
    {
        debugViewShader->Bind();
    }

    ID3D12DescriptorHeap* heaps[] = { descriptorHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += descriptorSize * descriptorSetOffset;
    cmdList->SetGraphicsRootDescriptorTable(0, gpuHandle);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList->IASetVertexBuffers(0, 1, &quadVertexBufferView);

    cmdList->DrawInstanced(4, 1, 0, 0);
}
