#pragma once
#include "ZNUtils.h"

namespace ZNFramework {

// A 6-face cube render target + one shared depth buffer (faces are captured sequentially,
// never simultaneously, so a single depth buffer reused across faces is sufficient) + one
// TEXTURECUBE SRV for sampling the result as an environment map.
class CubeRenderTexture {
public:
    void Init(uint32 size,
              DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
              DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT);

    ID3D12Resource*             GetResource()      const { return colorResource.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(uint32 face) const { return rtvHandles[face]; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV()            const { return dsvHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpuHandle()   const { return srvHandle; }

    uint32 GetSize() const { return size; }

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

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[6] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle     = {};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle     = {};

    uint32      size        = 0;
    DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
};

} // namespace ZNFramework
