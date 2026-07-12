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

    struct ToneMapCB
    {
        float bloomIntensity;
        float pad[3];
    };
}

ToneMappingPass::ToneMappingPass(RenderTexture* sceneColor, RenderTexture* bloom,
                                 SwapChain* swapChain, ZNShader* toneMapShader)
    : PostProcessPass("ToneMapping", { "SceneColor", "Bloom" }, "BackBuffer")
    , sceneColor(sceneColor), bloom(bloom), swapChain(swapChain), toneMapShader(toneMapShader)
{
    CreateFullscreenQuad();
    CreateConstantBuffer();
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

void ToneMappingPass::CreateConstantBuffer()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    uint32 bufferSize = (sizeof(ToneMapCB) + 255) & ~255;
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&constantBuffer)));

    constantBuffer->Map(0, nullptr, &mappedConstantBuffer);
}

void ToneMappingPass::CreateDescriptorHeap()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // Layout must match the root signature's single descriptor table (b0~b4, t0~t9):
    // b0 = tone mapping CB, t0 = SceneColor, t1 = Bloom.
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = TOTAL_DESCRIPTOR_TABLE_SIZE;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)));
}

void ToneMappingPass::Draw(ID3D12GraphicsCommandList* cmd, RenderGraph& rg,
                          const std::vector<RGResource*>& inputs, RGResource* output)
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

    ToneMapCB cbData = {};
    cbData.bloomIntensity = bloomIntensity;
    memcpy(mappedConstantBuffer, &cbData, sizeof(ToneMapCB));

    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    uint32 descSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE heapStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // b0: tone mapping CB (bloom intensity)
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = static_cast<UINT>(constantBuffer->GetDesc().Width);
    device->Device()->CreateConstantBufferView(&cbvDesc, heapStart);

    // t0: SceneColor, t1: Bloom
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heapStart;
    cpuHandle.ptr += static_cast<SIZE_T>(descSize) * CBV_REGISTER_COUNT;
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, sceneColor->GetSRVCpuHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cpuHandle.ptr += descSize;
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, bloom->GetSRVCpuHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ID3D12DescriptorHeap* heaps[] = { descriptorHeap.Get() };
    cmd->SetDescriptorHeaps(1, heaps);
    cmd->SetGraphicsRootDescriptorTable(0, descriptorHeap->GetGPUDescriptorHandleForHeapStart());

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmd->IASetVertexBuffers(0, 1, &quadVertexBufferView);
    cmd->DrawInstanced(4, 1, 0, 0);
}
