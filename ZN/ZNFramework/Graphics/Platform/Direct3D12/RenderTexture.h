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

    // Background/clear colour used when this RT is the target of an offscreen pass. Defaults to
    // black; a scene can set it (e.g. to match a uniform-grey studio environment) so offscreen
    // views aren't a black void. For a NON-uniform skybox, draw the actual skybox instead.
    void         SetClearColor(float r, float g, float b, float a = 1.0f) { clearColor[0]=r; clearColor[1]=g; clearColor[2]=b; clearColor[3]=a; }
    const float* GetClearColor() const { return clearColor; }

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
    float       clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
};

} // namespace ZNFramework
