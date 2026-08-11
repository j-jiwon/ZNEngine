#pragma once
#include "ZNUtils.h"

namespace ZNFramework {

// Converts an equirectangular (lat-long) panorama image into a static TextureCube,
// resampled on the CPU once at load time and uploaded as a single immutable resource
// (unlike CubeRenderTexture, this is never rendered into again after Init()).
class EquirectCubeTexture {
public:
    void Init(const std::wstring& path, uint32 faceSize, bool srgb);
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpuHandle() const { return srvHandle; }

private:
    ComPtr<ID3D12Resource> cubeResource;
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = {};
};

}
