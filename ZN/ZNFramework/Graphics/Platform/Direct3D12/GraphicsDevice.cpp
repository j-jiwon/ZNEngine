#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "CommandList.h"

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


ZNCommandQueue* GraphicsDevice::CreateCommandQueue()
{
    ComPtr<ID3D12CommandQueue> queue;
    {
        D3D12_COMMAND_QUEUE_DESC desc{};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
    }

    return new CommandQueue(this, queue.Get());
}

ZNCommandList* GraphicsDevice::CreateCommandList()
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    return new CommandList(this, commandAllocator.Get(), commandList.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void GraphicsDevice::UpdateCurrentFence()
{
    currentFence++;
}
