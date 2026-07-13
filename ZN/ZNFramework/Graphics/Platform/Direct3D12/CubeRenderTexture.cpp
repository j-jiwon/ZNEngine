#include "CubeRenderTexture.h"
#include "GraphicsDevice.h"
#include "ZNFramework.h"

using namespace ZNFramework;

void CubeRenderTexture::Init(uint32 inSize, uint32 inMipLevels, DXGI_FORMAT inColorFormat, DXGI_FORMAT inDepthFormat)
{
    size        = inSize;
    mipLevels   = inMipLevels;
    colorFormat = inColorFormat;
    depthFormat = inDepthFormat;

    CreateColorResource();
    CreateDepthResource();
    CreateRTV();
    CreateSRV();
    CreateDSV();
}

void CubeRenderTexture::CreateColorResource()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // DepthOrArraySize = 6: a cubemap is just a 2D texture array with 6 slices, tagged
    // TEXTURECUBE at the SRV level.
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
        colorFormat, size, size, 6, static_cast<UINT16>(mipLevels), 1, 0,
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

void CubeRenderTexture::CreateDepthResource()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
        depthFormat, size, size, 1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_CLEAR_VALUE clearValue    = CD3DX12_CLEAR_VALUE(depthFormat, 1.0f, 0);

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(&depthResource)));
}

void CubeRenderTexture::CreateRTV()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    uint32 count = mipLevels * 6;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = count;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap)));

    uint32 rtvSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

    rtvHandles.resize(count);

    for (uint32 mip = 0; mip < mipLevels; ++mip)
    {
        for (uint32 face = 0; face < 6; ++face)
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format                         = colorFormat;
            rtvDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice        = mip;
            rtvDesc.Texture2DArray.FirstArraySlice = face;
            rtvDesc.Texture2DArray.ArraySize       = 1;

            device->Device()->CreateRenderTargetView(colorResource.Get(), &rtvDesc, handle);
            rtvHandles[mip * 6 + face] = handle;
            handle.ptr += rtvSize;
        }
    }
}

void CubeRenderTexture::CreateSRV()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // CPU-side; copied into shader-visible heap per use

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvHeap)));
    srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                          = colorFormat;
    srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping          = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MostDetailedMip     = 0;
    srvDesc.TextureCube.MipLevels            = mipLevels;
    srvDesc.TextureCube.ResourceMinLODClamp  = 0.0f;

    device->Device()->CreateShaderResourceView(colorResource.Get(), &srvDesc, srvHandle);
}

void CubeRenderTexture::CreateDSV()
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
