#pragma once
#include "ZNUtils.h"

namespace ZNFramework
{
    class ShadowMap
    {
    public:
        void Init(uint32 width = 2048, uint32 height = 2048);
        void Resize(uint32 width, uint32 height);

        // DSV for shadow pass rendering
        D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return dsvHandle; }

        // SRV for sampling in lighting pass
        D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return srvHandle; }

        ID3D12Resource* GetResource() const { return shadowMapTexture.Get(); }
        ID3D12DescriptorHeap* GetSRVHeap() const { return srvHeap.Get(); }

        uint32 GetWidth() const { return width; }
        uint32 GetHeight() const { return height; }

    private:
        void CreateShadowMapResource();
        void CreateDSV();
        void CreateSRV();

    private:
        ComPtr<ID3D12Resource> shadowMapTexture;
        ComPtr<ID3D12DescriptorHeap> dsvHeap;
        ComPtr<ID3D12DescriptorHeap> srvHeap;

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = {};

        uint32 width = 2048;
        uint32 height = 2048;
    };
}
