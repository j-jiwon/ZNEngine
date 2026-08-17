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
        D3D12_CPU_DESCRIPTOR_HANDLE GetWorldPosRTV() const { return rtvHandles[3]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetARMRTV() const { return rtvHandles[4]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetEmissiveRTV() const { return rtvHandles[5]; }
        // SRV access for reading in debug views
        D3D12_CPU_DESCRIPTOR_HANDLE GetBaseColorSRV() const { return srvHandles[0]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetNormalSRV() const { return srvHandles[1]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthCopySRV() const { return srvHandles[2]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetWorldPosSRV() const { return srvHandles[3]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetARMSRV() const { return srvHandles[4]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetEmissiveSRV() const { return srvHandles[5]; }

        // Get all RTVs for OMSetRenderTargets
        D3D12_CPU_DESCRIPTOR_HANDLE* GetAllRTVs() { return rtvHandles; }
        uint32 GetRTVCount() const { return GBUFFER_COUNT; }

        ID3D12Resource* GetBaseColorResource() const { return gbufferTextures[0].Get(); }
        ID3D12Resource* GetNormalResource() const { return gbufferTextures[1].Get(); }
        ID3D12Resource* GetDepthCopyResource() const { return gbufferTextures[2].Get(); }
        ID3D12Resource* GetWorldPosResource() const { return gbufferTextures[3].Get(); }
        ID3D12Resource* GetARMResource() const { return gbufferTextures[4].Get(); }
        ID3D12Resource* GetEmissiveResource() const { return gbufferTextures[5].Get(); }
        ID3D12DescriptorHeap* GetSRVHeap() const { return srvHeap.Get(); }

        // Per-target optimised clear colour (index 0..5). Single source of truth shared by resource
        // creation and GBufferPass's ClearRenderTargetView — if the two drift, D3D12 falls back to
        // the slow non-optimised clear path ("clear values do not match" warning).
        static const float* ClearColor(uint32 index);

    private:
        void CreateGBufferResources();
        void CreateRTVs();
        void CreateSRVs();

    private:
        static constexpr uint32 GBUFFER_COUNT = 6;

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
