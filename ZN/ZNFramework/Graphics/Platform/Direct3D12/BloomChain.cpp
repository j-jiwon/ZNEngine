#include "BloomChain.h"
#include "GraphicsDevice.h"
#include "Shader.h"
#include "Graphics/ZNGraphicsContext.h"
#include "ZNFramework.h"

using namespace ZNFramework;

namespace {
    struct BloomVertex
    {
        float pos[3];
        float color[4];
        float uv[2];
        float normal[3];
    };

    struct ThresholdCB
    {
        float threshold;
        float pad[3];
    };
}

void BloomChain::Init(uint32 w, uint32 h)
{
    CreateFullscreenQuad();
    CreateShaders();
    CreateResources(w, h);
    CreateConstantBuffer();
    CreateDescriptorHeaps();
}

void BloomChain::Resize(uint32 w, uint32 h)
{
    CreateResources(w, h);
}

void BloomChain::CreateResources(uint32 w, uint32 h)
{
    width = w;
    height = h;

    uint32 mw = w, mh = h;
    for (uint32 i = 0; i < kNumMips; ++i)
    {
        mw = (mw > 0) ? mw : 1u;
        mh = (mh > 0) ? mh : 1u;
        if (mips[i].GetResource() == nullptr)
            mips[i].Init(mw, mh, DXGI_FORMAT_R16G16B16A16_FLOAT);
        else
            mips[i].Resize(mw, mh);

        mipState[i] = D3D12_RESOURCE_STATE_RENDER_TARGET; // matches RenderTexture's initial/post-resize state
        mw /= 2;
        mh /= 2;
    }
}

void BloomChain::CreateFullscreenQuad()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    BloomVertex vertices[] = {
        { {-1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { { 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { {-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
        { { 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }
    };

    uint32 bufferSize = sizeof(vertices);

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&quadVertexBuffer)));

    void* mappedData = nullptr;
    quadVertexBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, vertices, bufferSize);
    quadVertexBuffer->Unmap(0, nullptr);

    quadVertexBufferView.BufferLocation = quadVertexBuffer->GetGPUVirtualAddress();
    quadVertexBufferView.SizeInBytes = bufferSize;
    quadVertexBufferView.StrideInBytes = sizeof(BloomVertex);
}

void BloomChain::CreateShaders()
{
    DXGI_FORMAT hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    thresholdShader = new Shader();
    thresholdShader->Load(GetResourcePath() / L"Shaders" / L"bloom_threshold.hlsli");
    thresholdShader->DisableDepthTest();
    thresholdShader->SetRenderTargetFormats(1, &hdrFormat);

    downsampleShader = new Shader();
    downsampleShader->Load(GetResourcePath() / L"Shaders" / L"bloom_downsample.hlsli");
    downsampleShader->DisableDepthTest();
    downsampleShader->SetRenderTargetFormats(1, &hdrFormat);

    upsampleShader = new Shader();
    upsampleShader->Load(GetResourcePath() / L"Shaders" / L"bloom_upsample.hlsli");
    upsampleShader->DisableDepthTest();
    upsampleShader->SetRenderTargetFormats(1, &hdrFormat);
    upsampleShader->EnableAdditiveBlend();
}

void BloomChain::CreateConstantBuffer()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    uint32 bufferSize = (sizeof(ThresholdCB) + 255) & ~255;
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&thresholdConstantBuffer)));

    thresholdConstantBuffer->Map(0, nullptr, &mappedThresholdBuffer);
}

void BloomChain::CreateDescriptorHeaps()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = TOTAL_DESCRIPTOR_TABLE_SIZE; // matches root sig table: b0~b4, t0~t9
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&thresholdHeap)));
    for (uint32 i = 0; i < kNumMips - 1; ++i)
    {
        ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&downsampleHeaps[i])));
        ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&upsampleHeaps[i])));
    }
}

void BloomChain::TransitionMip(ID3D12GraphicsCommandList* cmd, uint32 index, D3D12_RESOURCE_STATES newState)
{
    if (mipState[index] == newState) return;
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mips[index].GetResource(), mipState[index], newState);
    cmd->ResourceBarrier(1, &barrier);
    mipState[index] = newState;
}

void BloomChain::DrawFullscreen(ID3D12GraphicsCommandList* cmd, Shader* shader,
                                D3D12_CPU_DESCRIPTOR_HANDLE srcSRV,
                                RenderTexture& dst, ID3D12DescriptorHeap* heap,
                                ID3D12Resource* cb)
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_VIEWPORT viewport   = { 0, 0, static_cast<FLOAT>(dst.GetWidth()), static_cast<FLOAT>(dst.GetHeight()), 0.f, 1.f };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(dst.GetWidth()), static_cast<LONG>(dst.GetHeight()) };
    cmd->RSSetViewports(1, &viewport);
    cmd->RSSetScissorRects(1, &scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = dst.GetRTV();
    cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    if (shader) shader->Bind();

    uint32 descSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE heapStart = heap->GetCPUDescriptorHandleForHeapStart();

    if (cb)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = cb->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes    = static_cast<UINT>(cb->GetDesc().Width);
        device->Device()->CreateConstantBufferView(&cbvDesc, heapStart);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heapStart;
    cpuHandle.ptr += static_cast<SIZE_T>(descSize) * CBV_REGISTER_COUNT;
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, srcSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ID3D12DescriptorHeap* heaps[] = { heap };
    cmd->SetDescriptorHeaps(1, heaps);
    cmd->SetGraphicsRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmd->IASetVertexBuffers(0, 1, &quadVertexBufferView);
    cmd->DrawInstanced(4, 1, 0, 0);
}

void BloomChain::Render(RenderTexture* sceneColor, ID3D12GraphicsCommandList* cmd)
{
    // Bright-pass extract: SceneColor -> mip0. mip0 is already RENDER_TARGET here —
    // BloomPass's PostProcessPass::Execute transitions the "Bloom" RGResource (the
    // same underlying resource as mips[0]) before Draw()/Render() is ever called.
    ThresholdCB cbData = {};
    cbData.threshold = threshold;
    memcpy(mappedThresholdBuffer, &cbData, sizeof(ThresholdCB));

    DrawFullscreen(cmd, thresholdShader, sceneColor->GetSRVCpuHandle(), mips[0], thresholdHeap.Get(), thresholdConstantBuffer.Get());

    // Downsample chain: mip[i] -> mip[i+1], full res down to the smallest mip.
    for (uint32 i = 0; i < kNumMips - 1; ++i)
    {
        TransitionMip(cmd, i, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        TransitionMip(cmd, i + 1, D3D12_RESOURCE_STATE_RENDER_TARGET);
        DrawFullscreen(cmd, downsampleShader, mips[i].GetSRVCpuHandle(), mips[i + 1], downsampleHeaps[i].Get());
    }

    // Upsample + additive-combine chain: mip[i+1] -> mip[i], smallest back up to mip0.
    // mip0's final value after this loop is the complete bloom result.
    for (int i = static_cast<int>(kNumMips) - 2; i >= 0; --i)
    {
        uint32 ui = static_cast<uint32>(i);
        TransitionMip(cmd, ui + 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        TransitionMip(cmd, ui, D3D12_RESOURCE_STATE_RENDER_TARGET);
        DrawFullscreen(cmd, upsampleShader, mips[ui + 1].GetSRVCpuHandle(), mips[ui], upsampleHeaps[ui].Get());
    }
}
