#include "ToneMappingPass.h"
#include "../GraphicsDevice.h"
#include "Graphics/ZNGraphicsContext.h"

using namespace ZNFramework;

namespace {
    struct ToneMapVertex
    {
        float pos[3];
        float color[4];
        float uv[2];
        float normal[3];
    };
}

ToneMappingPass::ToneMappingPass(RenderTexture* sceneColor, SwapChain* swapChain, ZNShader* toneMapShader)
    : PostProcessPass("ToneMapping", "SceneColor", "BackBuffer")
    , sceneColor(sceneColor), swapChain(swapChain), toneMapShader(toneMapShader)
{
    CreateFullscreenQuad();
    CreateDescriptorHeap();
}

void ToneMappingPass::CreateFullscreenQuad()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    ToneMapVertex vertices[] = {
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
    quadVertexBufferView.StrideInBytes = sizeof(ToneMapVertex);
}

void ToneMappingPass::CreateDescriptorHeap()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // Layout must match the root signature's single descriptor table (b0~b4, t0~t6);
    // tone mapping only ever populates the t0 slot (SceneColor).
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = CBV_REGISTER_COUNT + SRV_REGISTER_COUNT + 2;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)));
}

void ToneMappingPass::Draw(ID3D12GraphicsCommandList* cmd, RenderGraph& rg,
                          RGResource* input, RGResource* output)
{
    uint32 w = swapChain->Width(), h = swapChain->Height();
    D3D12_VIEWPORT viewport   = { 0, 0, static_cast<FLOAT>(w), static_cast<FLOAT>(h), 0.f, 1.f };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };
    cmd->RSSetViewports(1, &viewport);
    cmd->RSSetScissorRects(1, &scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChain->GetBackRTV();
    cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    if (toneMapShader)
        toneMapShader->Bind();

    // Copy SceneColor's SRV into the t0 slot of our dedicated table heap
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    uint32 descSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += static_cast<SIZE_T>(descSize) * CBV_REGISTER_COUNT;
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, sceneColor->GetSRVCpuHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ID3D12DescriptorHeap* heaps[] = { descriptorHeap.Get() };
    cmd->SetDescriptorHeaps(1, heaps);
    cmd->SetGraphicsRootDescriptorTable(0, descriptorHeap->GetGPUDescriptorHandleForHeapStart());

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmd->IASetVertexBuffers(0, 1, &quadVertexBufferView);
    cmd->DrawInstanced(4, 1, 0, 0);
}
