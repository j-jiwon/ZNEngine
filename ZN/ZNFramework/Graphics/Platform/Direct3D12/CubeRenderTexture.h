#pragma once
#include "ZNUtils.h"
#include <vector>

namespace ZNFramework {

// A 6-face cube render target + one shared depth buffer (faces are captured sequentially,
// never simultaneously, so a single depth buffer reused across faces is sufficient) + one
// TEXTURECUBE SRV for sampling the result as an environment map.
//
// Optionally supports multiple mip levels (mipLevels > 1) for a roughness-prefiltered
// specular chain: each mip gets its own 6 RTVs (one per face), and the SRV covers all
// mips so it can be sampled with SampleLevel() by roughness. Mip levels are still
// captured/rendered one face at a time, same as the base case.
class CubeRenderTexture {
public:
    void Init(uint32 size, uint32 mipLevels = 1,
              DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
              DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT);

    ID3D12Resource*             GetResource()                       const { return colorResource.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(uint32 face, uint32 mip = 0) const { return rtvHandles[mip * 6 + face]; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV()                            const { return dsvHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpuHandle()                   const { return srvHandle; }

    uint32 GetSize()       const { return size; }
    uint32 GetMipLevels()  const { return mipLevels; }

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

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles; // [mip * 6 + face]
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};

    uint32      size        = 0;
    uint32      mipLevels   = 1;
    DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
};

} // namespace ZNFramework
