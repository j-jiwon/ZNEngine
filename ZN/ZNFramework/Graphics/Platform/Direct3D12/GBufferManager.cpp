#include "GBufferManager.h"
#include "GraphicsDevice.h"
#include "ZNFramework.h"

using namespace ZNFramework;

void GBufferManager::Init(uint32 inWidth, uint32 inHeight)
{
    width = inWidth;
    height = inHeight;

    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    rtvDescriptorSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    srvDescriptorSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CreateGBufferResources();
    CreateRTVs();
    CreateSRVs();
}

void GBufferManager::CreateGBufferResources()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // Define formats for each G-Buffer
    DXGI_FORMAT formats[GBUFFER_COUNT] = {
        DXGI_FORMAT_R8G8B8A8_UNORM,      // Base Color
        DXGI_FORMAT_R16G16B16A16_FLOAT,  // World Normal (need precision)
        DXGI_FORMAT_R32_FLOAT            // Depth copy (for visualization)
    };

    // Create each G-Buffer texture
    for (int i = 0; i < GBUFFER_COUNT; ++i)
    {
        D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            formats[i],
            width,
            height,
            1, // Array size
            1, // Mip levels
            1, // Sample count
            0, // Sample quality
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
        );

        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        // Clear value for optimization (optional)
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = formats[i];
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;

        ThrowIfFailed(device->Device()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue,
            IID_PPV_ARGS(&gbufferTextures[i])
        ));
    }
}

void GBufferManager::CreateRTVs()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // Create RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = GBUFFER_COUNT;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

    // Create RTVs
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (int i = 0; i < GBUFFER_COUNT; ++i)
    {
        device->Device()->CreateRenderTargetView(gbufferTextures[i].Get(), nullptr, rtvHandle);
        rtvHandles[i] = rtvHandle;
        rtvHandle.ptr += rtvDescriptorSize;
    }
}

void GBufferManager::CreateSRVs()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // Create SRV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = GBUFFER_COUNT;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)));

    // Create SRVs
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();

    for (int i = 0; i < GBUFFER_COUNT; ++i)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = gbufferTextures[i]->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        device->Device()->CreateShaderResourceView(gbufferTextures[i].Get(), &srvDesc, srvHandle);
        srvHandles[i] = srvHandle;
        srvHandle.ptr += srvDescriptorSize;
    }
}

void GBufferManager::Clear()
{
    // Not needed for now
}

void GBufferManager::Resize(uint32 inWidth, uint32 inHeight)
{
    // Release old resources
    for (int i = 0; i < GBUFFER_COUNT; ++i)
    {
        gbufferTextures[i].Reset();
    }
    rtvHeap.Reset();
    srvHeap.Reset();

    // Reinitialize with new size
    Init(inWidth, inHeight);
}
