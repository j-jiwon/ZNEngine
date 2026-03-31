#pragma once
#include "ZNUtils.h"

namespace ZNFramework
{
    class GBufferManager
    {
    public:
        void Init(uint32 width, uint32 height);
        void Resize(uint32 width, uint32 height);
        void Clear();

        // RTV access for writing during geometry pass
        D3D12_CPU_DESCRIPTOR_HANDLE GetBaseColorRTV() const { return rtvHandles[0]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetNormalRTV() const { return rtvHandles[1]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthCopyRTV() const { return rtvHandles[2]; }

        // SRV access for reading in debug views
        D3D12_CPU_DESCRIPTOR_HANDLE GetBaseColorSRV() const { return srvHandles[0]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetNormalSRV() const { return srvHandles[1]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthCopySRV() const { return srvHandles[2]; }

        // Get all RTVs for OMSetRenderTargets
        D3D12_CPU_DESCRIPTOR_HANDLE* GetAllRTVs() { return rtvHandles; }
        uint32 GetRTVCount() const { return GBUFFER_COUNT; }

        ID3D12Resource* GetBaseColorResource() const { return gbufferTextures[0].Get(); }
        ID3D12Resource* GetNormalResource() const { return gbufferTextures[1].Get(); }
        ID3D12Resource* GetDepthCopyResource() const { return gbufferTextures[2].Get(); }

        ID3D12DescriptorHeap* GetSRVHeap() const { return srvHeap.Get(); }

    private:
        void CreateGBufferResources();
        void CreateRTVs();
        void CreateSRVs();

    private:
        static constexpr uint32 GBUFFER_COUNT = 3;

        ComPtr<ID3D12Resource> gbufferTextures[GBUFFER_COUNT];
        ComPtr<ID3D12DescriptorHeap> rtvHeap;
        ComPtr<ID3D12DescriptorHeap> srvHeap;

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[GBUFFER_COUNT];
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandles[GBUFFER_COUNT];

        uint32 width = 0;
        uint32 height = 0;
        uint32 rtvDescriptorSize = 0;
        uint32 srvDescriptorSize = 0;
    };
}
