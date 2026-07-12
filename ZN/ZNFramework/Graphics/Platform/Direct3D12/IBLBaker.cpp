#include "IBLBaker.h"
#include "GraphicsDevice.h"
#include "Shader.h"
#include "CommandQueue.h"
#include "Graphics/ZNGraphicsContext.h"
#include "ZNFramework.h"
#include "../../../Math/ZNVector3.h"

using namespace ZNFramework;

namespace {
    struct IBLVertex
    {
        float pos[3];
        float color[4];
        float uv[2];
        float normal[3];
    };

    // Same per-face look/up basis as ZNScene::AddCubemapCapture's kFaces table (must
    // match — these bake into the same kind of TextureCube the real camera-based
    // CubeCapturePass populates, so face N here must mean the same direction as face N
    // there). forward/right/up below are then derived with the exact same formula
    // ZNCamera::SetView(pos, target, up) uses internally (DirectX LookAtLH-style:
    // zAxis=dir, xAxis=cross(up,zAxis), yAxis=cross(zAxis,xAxis)) — reimplemented here
    // directly (rather than round-tripping through a real ZNCamera) since ZNCamera's
    // GetForward()/GetRight()/GetUp() members are only refreshed by the pitch/yaw path,
    // not by the SetView(pos,target,up) overload.
    struct FaceBasis { ZNVector3 dir; ZNVector3 up; };
    static const FaceBasis kFaces[6] = {
        { ZNVector3( 1.f,  0.f,  0.f), ZNVector3(0.f, 1.f,  0.f) }, // +X
        { ZNVector3(-1.f,  0.f,  0.f), ZNVector3(0.f, 1.f,  0.f) }, // -X
        { ZNVector3( 0.f,  1.f,  0.f), ZNVector3(0.f, 0.f, -1.f) }, // +Y
        { ZNVector3( 0.f, -1.f,  0.f), ZNVector3(0.f, 0.f,  1.f) }, // -Y
        { ZNVector3( 0.f,  0.f,  1.f), ZNVector3(0.f, 1.f,  0.f) }, // +Z
        { ZNVector3( 0.f,  0.f, -1.f), ZNVector3(0.f, 1.f,  0.f) }, // -Z
    };

    void ComputeFaceBasis(uint32 face, ZNVector3& outForward, ZNVector3& outRight, ZNVector3& outUp)
    {
        ZNVector3 zAxis = kFaces[face].dir; // already unit length
        ZNVector3 xAxis = ZNVector3::Cross(kFaces[face].up, zAxis).Normalize();
        ZNVector3 yAxis = ZNVector3::Cross(zAxis, xAxis).Normalize();
        outForward = zAxis;
        outRight   = xAxis;
        outUp      = yAxis;
    }
}

void IBLBaker::Init()
{
    CreateFullscreenQuad();
    CreateShaders();

    irradianceCube.Init(kIrradianceSize, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
    prefilteredCube.Init(kPrefilterSize, kPrefilterMips, DXGI_FORMAT_R16G16B16A16_FLOAT);
    brdfLUT.Init(kBRDFLUTSize, kBRDFLUTSize, DXGI_FORMAT_R16G16B16A16_FLOAT);

    CreateFallbackCubes();

    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    uint32 cbSize = (sizeof(FaceCB) + 255) & ~255;
    D3D12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

    for (uint32 i = 0; i < 6; ++i)
    {
        ThrowIfFailed(device->Device()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&irradianceCB[i])));
        irradianceCB[i]->Map(0, nullptr, &mappedIrradianceCB[i]);
    }

    for (uint32 i = 0; i < kNumPrefilterDraws; ++i)
    {
        ThrowIfFailed(device->Device()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&prefilterCB[i])));
        prefilterCB[i]->Map(0, nullptr, &mappedPrefilterCB[i]);
    }

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = TOTAL_DESCRIPTOR_TABLE_SIZE;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    for (uint32 i = 0; i < 6; ++i)
        ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&irradianceHeaps[i])));

    for (uint32 i = 0; i < kNumPrefilterDraws; ++i)
        ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&prefilterHeaps[i])));

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&brdfLUTHeap)));
}

void IBLBaker::CreateFullscreenQuad()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    IBLVertex vertices[] = {
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
    quadVertexBufferView.StrideInBytes = sizeof(IBLVertex);
}

void IBLBaker::CreateShaders()
{
    DXGI_FORMAT hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    irradianceShader = new Shader();
    irradianceShader->Load(GetResourcePath() / L"Shaders" / L"ibl_irradiance.hlsli");
    irradianceShader->DisableDepthTest();
    irradianceShader->SetRenderTargetFormats(1, &hdrFormat);

    prefilterShader = new Shader();
    prefilterShader->Load(GetResourcePath() / L"Shaders" / L"ibl_prefilter.hlsli");
    prefilterShader->DisableDepthTest();
    prefilterShader->SetRenderTargetFormats(1, &hdrFormat);

    brdfLUTShader = new Shader();
    brdfLUTShader->Load(GetResourcePath() / L"Shaders" / L"ibl_brdf_lut.hlsli");
    brdfLUTShader->DisableDepthTest();
    brdfLUTShader->SetRenderTargetFormats(1, &hdrFormat);
}

void IBLBaker::CreateFallbackCubes()
{
    fallbackIrradianceCube.Init(1);
    fallbackPrefilteredCube.Init(1);

    CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    ID3D12GraphicsCommandList* cmd = queue->ResourceCommandList();
    float black[4] = { 0.f, 0.f, 0.f, 1.f };
    for (uint32 face = 0; face < 6; ++face)
    {
        cmd->ClearRenderTargetView(fallbackIrradianceCube.GetRTV(face), black, 0, nullptr);
        cmd->ClearRenderTargetView(fallbackPrefilteredCube.GetRTV(face), black, 0, nullptr);
    }
    CD3DX12_RESOURCE_BARRIER barriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(fallbackIrradianceCube.GetResource(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(fallbackPrefilteredCube.GetResource(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    };
    cmd->ResourceBarrier(2, barriers);
    queue->FlushResourceQueue();
}

void IBLBaker::DrawFace(ID3D12GraphicsCommandList* cmd, Shader* shader,
                       D3D12_CPU_DESCRIPTOR_HANDLE srcSRV, D3D12_CPU_DESCRIPTOR_HANDLE dstRTV,
                       uint32 faceSize, const FaceCB& cbData,
                       ID3D12Resource* cbResource, void* mappedCB, ID3D12DescriptorHeap* heap)
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    memcpy(mappedCB, &cbData, sizeof(FaceCB));

    D3D12_VIEWPORT viewport    = { 0, 0, static_cast<FLOAT>(faceSize), static_cast<FLOAT>(faceSize), 0.f, 1.f };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(faceSize), static_cast<LONG>(faceSize) };
    cmd->RSSetViewports(1, &viewport);
    cmd->RSSetScissorRects(1, &scissorRect);
    cmd->OMSetRenderTargets(1, &dstRTV, FALSE, nullptr);

    if (shader) shader->Bind();

    uint32 descSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE heapStart = heap->GetCPUDescriptorHandleForHeapStart();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = cbResource->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes    = static_cast<UINT>(cbResource->GetDesc().Width);
    device->Device()->CreateConstantBufferView(&cbvDesc, heapStart);

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

void IBLBaker::Draw2D(ID3D12GraphicsCommandList* cmd, Shader* shader,
                     RenderTexture& dst, ID3D12DescriptorHeap* heap)
{
    D3D12_VIEWPORT viewport    = { 0, 0, static_cast<FLOAT>(dst.GetWidth()), static_cast<FLOAT>(dst.GetHeight()), 0.f, 1.f };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(dst.GetWidth()), static_cast<LONG>(dst.GetHeight()) };
    cmd->RSSetViewports(1, &viewport);
    cmd->RSSetScissorRects(1, &scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = dst.GetRTV();
    cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    if (shader) shader->Bind();

    // No CBV/SRV needed — ibl_brdf_lut.hlsli is pure UV-driven math. Root sig still
    // requires a bound descriptor table, so the heap is bound but left unpopulated.
    ID3D12DescriptorHeap* heaps[] = { heap };
    cmd->SetDescriptorHeaps(1, heaps);
    cmd->SetGraphicsRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmd->IASetVertexBuffers(0, 1, &quadVertexBufferView);
    cmd->DrawInstanced(4, 1, 0, 0);
}

void IBLBaker::BakeBRDFLUT(ID3D12GraphicsCommandList* cmd)
{
    if (brdfBaked) return;

    Draw2D(cmd, brdfLUTShader, brdfLUT, brdfLUTHeap.Get());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        brdfLUT.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmd->ResourceBarrier(1, &barrier);

    brdfBaked = true;
}

void IBLBaker::UpdateEnvironment(bool hasEnvSource, D3D12_CPU_DESCRIPTOR_HANDLE srcEnvCubeSRV, ID3D12GraphicsCommandList* cmd)
{
    envActive = hasEnvSource;
    if (!hasEnvSource) return;

    if (hasBakedAny && lastBakedSRV.ptr == srcEnvCubeSRV.ptr) return; // already baked this exact source

    if (hasBakedAny)
    {
        // Re-baking a different source: both cubes are currently PIXEL_SHADER_RESOURCE
        // (left there by the previous bake) — flip back to RENDER_TARGET before writing.
        CD3DX12_RESOURCE_BARRIER barriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(irradianceCube.GetResource(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
            CD3DX12_RESOURCE_BARRIER::Transition(prefilteredCube.GetResource(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
        };
        cmd->ResourceBarrier(2, barriers);
    }

    // Irradiance: one draw per face, full hemisphere convolution.
    for (uint32 face = 0; face < 6; ++face)
    {
        ZNVector3 forward, right, up;
        ComputeFaceBasis(face, forward, right, up);

        FaceCB cb = {};
        cb.forward[0] = forward.x; cb.forward[1] = forward.y; cb.forward[2] = forward.z;
        cb.right[0]   = right.x;   cb.right[1]   = right.y;   cb.right[2]   = right.z;
        cb.up[0]      = up.x;      cb.up[1]      = up.y;      cb.up[2]      = up.z;
        cb.roughness  = 0.0f; // unused by ibl_irradiance.hlsli

        DrawFace(cmd, irradianceShader, srcEnvCubeSRV, irradianceCube.GetRTV(face),
                kIrradianceSize, cb, irradianceCB[face].Get(), mappedIrradianceCB[face], irradianceHeaps[face].Get());
    }
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            irradianceCube.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmd->ResourceBarrier(1, &barrier);
    }

    // Prefiltered specular: one draw per (face, mip), roughness = mip / (mipCount-1).
    for (uint32 mip = 0; mip < kPrefilterMips; ++mip)
    {
        uint32 mipSize = kPrefilterSize >> mip;
        float roughness = static_cast<float>(mip) / static_cast<float>(kPrefilterMips - 1);

        for (uint32 face = 0; face < 6; ++face)
        {
            ZNVector3 forward, right, up;
            ComputeFaceBasis(face, forward, right, up);

            uint32 drawIdx = mip * 6 + face;

            FaceCB cb = {};
            cb.forward[0] = forward.x; cb.forward[1] = forward.y; cb.forward[2] = forward.z;
            cb.right[0]   = right.x;   cb.right[1]   = right.y;   cb.right[2]   = right.z;
            cb.up[0]      = up.x;      cb.up[1]      = up.y;      cb.up[2]      = up.z;
            cb.roughness  = roughness;

            DrawFace(cmd, prefilterShader, srcEnvCubeSRV, prefilteredCube.GetRTV(face, mip),
                    mipSize, cb, prefilterCB[drawIdx].Get(), mappedPrefilterCB[drawIdx], prefilterHeaps[drawIdx].Get());
        }
    }
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            prefilteredCube.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmd->ResourceBarrier(1, &barrier);
    }

    lastBakedSRV = srcEnvCubeSRV;
    hasBakedAny = true;
}
