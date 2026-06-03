#include "ShadowMap.h"
#include "GraphicsDevice.h"
#include "ZNFramework.h"

using namespace ZNFramework;

void ShadowMap::Init(uint32 inWidth, uint32 inHeight)
{
    width = inWidth;
    height = inHeight;

    CreateShadowMapResource();
    CreateDSV();
    CreateSRV();
}

void ShadowMap::CreateShadowMapResource()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // Use TYPELESS format for dual-use (DSV + SRV)
    D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32_TYPELESS,
        width,
        height,
        1,  // Array size
        1,  // Mip levels
        1,  // Sample count
        0,  // Sample quality
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    );

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    // Clear value for depth
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&shadowMapTexture)
    ));
}

void ShadowMap::CreateDSV()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // Create DSV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsvHeap)));

    // Create DSV
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    device->Device()->CreateDepthStencilView(shadowMapTexture.Get(), &dsvDesc, dsvHandle);
}

void ShadowMap::CreateSRV()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // Create SRV descriptor heap (non-shader-visible for CPU copy source)
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvHeap)));

    // Create SRV with R32_FLOAT format for shader sampling
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    device->Device()->CreateShaderResourceView(shadowMapTexture.Get(), &srvDesc, srvHandle);
}

void ShadowMap::Resize(uint32 inWidth, uint32 inHeight)
{
    // Release old resources
    shadowMapTexture.Reset();
    dsvHeap.Reset();
    srvHeap.Reset();

    // Reinitialize with new size
    Init(inWidth, inHeight);
}
