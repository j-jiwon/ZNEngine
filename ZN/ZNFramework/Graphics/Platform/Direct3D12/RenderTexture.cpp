#include "RenderTexture.h"
#include "GraphicsDevice.h"
#include "ZNFramework.h"

using namespace ZNFramework;

void RenderTexture::Init(uint32 inWidth, uint32 inHeight,
                         DXGI_FORMAT inColorFormat, DXGI_FORMAT inDepthFormat)
{
    width       = inWidth;
    height      = inHeight;
    colorFormat = inColorFormat;
    depthFormat = inDepthFormat;

    CreateColorResource();
    CreateDepthResource();
    CreateRTV();
    CreateSRV();
    CreateDSV();
}

void RenderTexture::Resize(uint32 inWidth, uint32 inHeight)
{
    colorResource.Reset();
    depthResource.Reset();
    rtvHeap.Reset();
    srvHeap.Reset();
    dsvHeap.Reset();

    width  = inWidth;
    height = inHeight;

    CreateColorResource();
    CreateDepthResource();
    CreateRTV();
    CreateSRV();
    CreateDSV();
}

void RenderTexture::CreateColorResource()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
        colorFormat, width, height, 1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format   = colorFormat;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
        IID_PPV_ARGS(&colorResource)));
}

void RenderTexture::CreateDepthResource()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
        depthFormat, width, height, 1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_CLEAR_VALUE clearValue    = CD3DX12_CLEAR_VALUE(depthFormat, 1.0f, 0);

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(&depthResource)));
}

void RenderTexture::CreateRTV()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap)));
    rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    device->Device()->CreateRenderTargetView(colorResource.Get(), nullptr, rtvHandle);
}

void RenderTexture::CreateSRV()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // CPU-side; copied into shader-visible heap per frame

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvHeap)));
    srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                    = colorFormat;
    srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels       = 1;

    device->Device()->CreateShaderResourceView(colorResource.Get(), &srvDesc, srvHandle);
}

void RenderTexture::CreateDSV()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsvHeap)));
    dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    device->Device()->CreateDepthStencilView(depthResource.Get(), nullptr, dsvHandle);
}
