#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "SwapChain.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNGraphicsDevice* CreateGraphicsDevice()
	{
		return new GraphicsDevice();
	}
}

GraphicsDevice::GraphicsDevice()
{
#ifdef _DEBUG
    ::D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    debugController->EnableDebugLayer();
#endif

    //device.Reset();
    //fence.Reset();

    CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
    //ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    // Find the adapter that matches the created device via LUID, then QI for IDXGIAdapter3
    LUID luid = device->GetAdapterLuid();
    ComPtr<IDXGIAdapter1> adapter1;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter1->GetDesc1(&desc);
        if (desc.AdapterLuid.LowPart == luid.LowPart && desc.AdapterLuid.HighPart == luid.HighPart)
        {
            adapter1.As(&adapter3);
            break;
        }
    }
}

float GraphicsDevice::GetGpuMemoryUsageMB() const
{
    if (!adapter3) return 0.0f;
    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
    return static_cast<float>(info.CurrentUsage) / (1024.0f * 1024.0f);
}

float GraphicsDevice::GetGpuMemoryBudgetMB() const
{
    if (!adapter3) return 0.0f;
    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
    return static_cast<float>(info.Budget) / (1024.0f * 1024.0f);
}
