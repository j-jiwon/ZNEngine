#include "CommandQueue.h"
#include "SwapChain.h"
#include "CommandList.h"
#include "DescriptorHeap.h"

using namespace ZNFramework;

CommandQueue::CommandQueue(GraphicsDevice* device, ID3D12CommandQueue* queue)
	: queue(queue)
	, device(device)
{
	descHeap = new DescriptorHeap();
}

ZNSwapChain* CommandQueue::CreateSwapChain(const ZNWindow* window)
{
	swapChain = new SwapChain(device, this, window);
	return swapChain;
}

void CommandQueue::Init()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	device->Device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
	device->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	device->Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	
	commandList->Close();

	device->Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

	swapChain->Init();
	descHeap->Init(device, swapChain);
}

void CommandQueue::RenderBegin()
{
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChain->GetCurrentBackBufferResource().Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	commandList->ResourceBarrier(1, &barrier);

	D3D12_VIEWPORT vp = { 0, 0, static_cast<FLOAT>(swapChain->Width()), static_cast<FLOAT>(swapChain->Height()) };
	D3D12_RECT rect = CD3DX12_RECT(0, 0, swapChain->Width(), swapChain->Height());

	commandList->RSSetViewports(1, &vp);
	commandList->RSSetScissorRects(1, &rect);

	// Specify the buffers we are going to render to.
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = descHeap->GetBackBufferView();
	float lightSteelBlue[] = { 1, 0, 0, 1 };
	commandList->ClearRenderTargetView(backBufferView, lightSteelBlue, 0, nullptr);
	commandList->OMSetRenderTargets(1, &backBufferView, FALSE, nullptr);
}

void CommandQueue::RenderEnd()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChain->GetCurrentBackBufferResource().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	commandList->ResourceBarrier(1, &barrier);
	ThrowIfFailed(commandList->Close());

	ID3D12CommandList* cmdListArr[] = { commandList.Get() };
	queue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	swapChain->Present();

	WaitSync();

	swapChain->SwapIndex();
}

void CommandQueue::OnResize(size_t inWidth, size_t inHeight)
{
	descHeap->OnResize(inWidth, inHeight);
}

void CommandQueue::WaitSync()
{
	fenceValue++;
	queue->Signal(fence.Get(), fenceValue);

	if (fence->GetCompletedValue() < fenceValue)
	{
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		::WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void CommandQueue::Enqueue(CommandList* commandList)
{
	//ID3D12CommandList* cmdLists[] = { commandList->List() };
	//Queue()->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}
