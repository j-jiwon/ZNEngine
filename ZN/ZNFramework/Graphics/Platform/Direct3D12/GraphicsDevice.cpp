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

    device.Reset();
    fence.Reset();

    CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
    //ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}
