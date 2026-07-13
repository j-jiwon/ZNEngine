#include "SkyboxRenderer.h"
#include "GraphicsDevice.h"
#include "Shader.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "TableDescriptorHeap.h"
#include "Graphics/ZNGraphicsContext.h"
#include "ZNFramework.h"
#include "../../../Math/ZNVector3.h"
#include "../../../ZNCamera.h"

using namespace ZNFramework;

namespace {
    struct SkyboxVertex
    {
        float pos[3];
        float color[4];
        float uv[2];
        float normal[3];
    };

    // Same per-face look/up basis as ZNScene::AddCubemapCapture's kFaces table (also
    // duplicated in EquirectCubeTexture.cpp and IBLBaker.cpp) — must match, since this
    // fills the same kind of TextureCube those populate. forward/right/up derived with
    // the exact formula ZNCamera::SetView(pos,target,up) uses internally (verified
    // during the IBL work): zAxis=dir, xAxis=cross(up,zAxis), yAxis=cross(zAxis,xAxis).
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

void SkyboxRenderer::Init()
{
    CreateFullscreenQuad();
    CreateShaders();
    CreateConstantBuffers();
    CreateDescriptorHeaps();
    CreateFallbackCube();
}

void SkyboxRenderer::CreateFullscreenQuad()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    SkyboxVertex vertices[] = {
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
    quadVertexBufferView.StrideInBytes = sizeof(SkyboxVertex);
}

void SkyboxRenderer::CreateShaders()
{
    resolveShader = new Shader();
    resolveShader->Load(GetResourcePath() / L"Shaders" / L"skybox_resolve.hlsli");
    resolveShader->DisableDepthTest();
    DXGI_FORMAT hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    resolveShader->SetRenderTargetFormats(1, &hdrFormat);

    backgroundShader = new Shader();
    backgroundShader->Load(GetResourcePath() / L"Shaders" / L"skybox_background.hlsli");
    backgroundShader->DisableDepthTest();
    DXGI_FORMAT ldrFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    backgroundShader->SetRenderTargetFormats(1, &ldrFormat);
}

void SkyboxRenderer::CreateConstantBuffers()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    {
        uint32 cbSize = (sizeof(ResolveCB) + 255) & ~255;
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
        ThrowIfFailed(device->Device()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resolveCB)));
        resolveCB->Map(0, nullptr, &mappedResolveCB);
    }

    uint32 bgCbSize = (sizeof(BackgroundCB) + 255) & ~255;
    D3D12_RESOURCE_DESC bgDesc = CD3DX12_RESOURCE_DESC::Buffer(bgCbSize);
    for (uint32 i = 0; i < 6; ++i)
    {
        ThrowIfFailed(device->Device()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &bgDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&backgroundCB[i])));
        backgroundCB[i]->Map(0, nullptr, &mappedBackgroundCB[i]);
    }
}

void SkyboxRenderer::CreateDescriptorHeaps()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = TOTAL_DESCRIPTOR_TABLE_SIZE;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&resolveHeap)));
    for (uint32 i = 0; i < 6; ++i)
        ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&backgroundHeaps[i])));
}

void SkyboxRenderer::CreateFallbackCube()
{
    fallbackCube.Init(1);

    CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    ID3D12GraphicsCommandList* cmd = queue->ResourceCommandList();
    float black[4] = { 0.f, 0.f, 0.f, 1.f };
    for (uint32 face = 0; face < 6; ++face)
        cmd->ClearRenderTargetView(fallbackCube.GetRTV(face), black, 0, nullptr);
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        fallbackCube.GetResource(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmd->ResourceBarrier(1, &barrier);
    queue->FlushResourceQueue();
}

void SkyboxRenderer::DrawResolve(ID3D12GraphicsCommandList* cmd, ZNCamera* camera,
                                 D3D12_CPU_DESCRIPTOR_HANDLE depthSRV,
                                 bool hasSkybox, D3D12_CPU_DESCRIPTOR_HANDLE skyboxSRV,
                                 D3D12_CPU_DESCRIPTOR_HANDLE dstRTV, uint32 width, uint32 height)
{
    if (!camera) return;

    ZNVector3 fwd = camera->GetForward();
    ZNVector3 rgt = camera->GetRight();
    ZNVector3 up  = camera->GetUp();
    ZNMatrix4 proj = camera->ProjectionMatrix();

    ResolveCB cb = {};
    cb.forward[0] = fwd.x; cb.forward[1] = fwd.y; cb.forward[2] = fwd.z;
    cb.right[0]   = rgt.x; cb.right[1]   = rgt.y; cb.right[2]   = rgt.z;
    cb.up[0]      = up.x;  cb.up[1]      = up.y;  cb.up[2]      = up.z;
    cb.tanHalfFovX = (proj._11 != 0.0f) ? 1.0f / proj._11 : 1.0f;
    cb.tanHalfFovY = (proj._22 != 0.0f) ? 1.0f / proj._22 : 1.0f;
    memcpy(mappedResolveCB, &cb, sizeof(ResolveCB));

    D3D12_VIEWPORT viewport    = { 0, 0, static_cast<FLOAT>(width), static_cast<FLOAT>(height), 0.f, 1.f };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    cmd->RSSetViewports(1, &viewport);
    cmd->RSSetScissorRects(1, &scissorRect);
    cmd->OMSetRenderTargets(1, &dstRTV, FALSE, nullptr);

    if (resolveShader) resolveShader->Bind();

    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    uint32 descSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE heapStart = resolveHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = resolveCB->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes    = static_cast<UINT>(resolveCB->GetDesc().Width);
    device->Device()->CreateConstantBufferView(&cbvDesc, heapStart);

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heapStart;
    cpuHandle.ptr += static_cast<SIZE_T>(descSize) * CBV_REGISTER_COUNT;
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, depthSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cpuHandle.ptr += descSize;
    D3D12_CPU_DESCRIPTOR_HANDLE actualSkySRV = hasSkybox ? skyboxSRV : fallbackCube.GetSRVCpuHandle();
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, actualSkySRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ID3D12DescriptorHeap* heaps[] = { resolveHeap.Get() };
    cmd->SetDescriptorHeaps(1, heaps);
    cmd->SetGraphicsRootDescriptorTable(0, resolveHeap->GetGPUDescriptorHandleForHeapStart());

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmd->IASetVertexBuffers(0, 1, &quadVertexBufferView);
    cmd->DrawInstanced(4, 1, 0, 0);
}

void SkyboxRenderer::DrawBackground(ID3D12GraphicsCommandList* cmd, uint32 faceIndex,
                                    bool hasSkybox, D3D12_CPU_DESCRIPTOR_HANDLE skyboxSRV,
                                    D3D12_CPU_DESCRIPTOR_HANDLE dstRTV, uint32 faceSize)
{
    ZNVector3 forward, right, up;
    ComputeFaceBasis(faceIndex, forward, right, up);

    BackgroundCB cb = {};
    cb.forward[0] = forward.x; cb.forward[1] = forward.y; cb.forward[2] = forward.z;
    cb.right[0]   = right.x;   cb.right[1]   = right.y;   cb.right[2]   = right.z;
    cb.up[0]      = up.x;      cb.up[1]      = up.y;      cb.up[2]      = up.z;
    memcpy(mappedBackgroundCB[faceIndex], &cb, sizeof(BackgroundCB));

    D3D12_VIEWPORT viewport    = { 0, 0, static_cast<FLOAT>(faceSize), static_cast<FLOAT>(faceSize), 0.f, 1.f };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(faceSize), static_cast<LONG>(faceSize) };
    cmd->RSSetViewports(1, &viewport);
    cmd->RSSetScissorRects(1, &scissorRect);
    cmd->OMSetRenderTargets(1, &dstRTV, FALSE, nullptr);

    if (backgroundShader) backgroundShader->Bind();

    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    uint32 descSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    ID3D12DescriptorHeap* heap = backgroundHeaps[faceIndex].Get();
    D3D12_CPU_DESCRIPTOR_HANDLE heapStart = heap->GetCPUDescriptorHandleForHeapStart();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = backgroundCB[faceIndex]->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes    = static_cast<UINT>(backgroundCB[faceIndex]->GetDesc().Width);
    device->Device()->CreateConstantBufferView(&cbvDesc, heapStart);

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heapStart;
    cpuHandle.ptr += static_cast<SIZE_T>(descSize) * CBV_REGISTER_COUNT;
    D3D12_CPU_DESCRIPTOR_HANDLE actualSkySRV = hasSkybox ? skyboxSRV : fallbackCube.GetSRVCpuHandle();
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, actualSkySRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ID3D12DescriptorHeap* heaps[] = { heap };
    cmd->SetDescriptorHeaps(1, heaps);
    cmd->SetGraphicsRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmd->IASetVertexBuffers(0, 1, &quadVertexBufferView);
    cmd->DrawInstanced(4, 1, 0, 0);

    // Restore the shared root signature + table descriptor heap: the caller's per-object
    // rendering that follows this (each face's real scene geometry) expects the shared
    // heap already bound, same as ForwardRenderPass/OffscreenCameraPass's own restore.
    RootSignature* rootSig = GraphicsContext::GetInstance().GetAs<RootSignature>();
    TableDescriptorHeap* tdh = GraphicsContext::GetInstance().GetAs<TableDescriptorHeap>();
    if (rootSig && tdh)
    {
        cmd->SetGraphicsRootSignature(rootSig->GetSignature().Get());
        ID3D12DescriptorHeap* sharedHeap = tdh->GetDescriptorHeap().Get();
        cmd->SetDescriptorHeaps(1, &sharedHeap);
    }
}
