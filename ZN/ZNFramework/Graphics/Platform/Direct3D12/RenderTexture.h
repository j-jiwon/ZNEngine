#pragma once
#include "ZNUtils.h"

namespace ZNFramework {

class RenderTexture {
public:
    void Init(uint32 width, uint32 height,
              DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
              DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT);

    void Resize(uint32 width, uint32 height);

    ID3D12Resource*             GetResource()     const { return colorResource.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV()          const { return rtvHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpuHandle() const { return srvHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV()          const { return dsvHandle; }

    uint32       GetWidth()       const { return width; }
    uint32       GetHeight()      const { return height; }
    DXGI_FORMAT  GetColorFormat() const { return colorFormat; }

private:
    void CreateColorResource();
    void CreateDepthResource();
    void CreateRTV();
    void CreateSRV();
    void CreateDSV();

    ComPtr<ID3D12Resource> colorResource;
    ComPtr<ID3D12Resource> depthResource;

    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};

    uint32      width       = 0;
    uint32      height      = 0;
    DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
};

} // namespace ZNFramework
