#include "CommandQueue.h"
#include "SwapChain.h"
#include "RootSignature.h"
#include "ConstantBuffer.h"
#include "GraphicsDevice.h"
#include "TableDescriptorHeap.h"
#include "ZNFramework.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNCommandQueue* CreateCommandQueue()
	{
		return new CommandQueue();
	}
}

void CommandQueue::Init(ZNSwapChain* inSwapChain)
{
	// init variables
	device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	swapChain = dynamic_cast<SwapChain*>(inSwapChain);

	// init
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	device->Device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
	device->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	device->Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	commandList->Close();

	device->Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void CommandQueue::RenderBegin()
{
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChain->GetBackRTVBuffer().Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	RootSignature* rootSignature = GraphicsContext::GetInstance().GetAs<RootSignature>();
	commandList->SetGraphicsRootSignature(rootSignature->GetSignature().Get());
	ConstantBuffer* constantBuffer = GraphicsContext::GetInstance().GetAs<ConstantBuffer>();
	constantBuffer->Clear();
	TableDescriptorHeap* tableDescHeap = GraphicsContext::GetInstance().GetAs<TableDescriptorHeap>();
	tableDescHeap->Clear();
	ID3D12DescriptorHeap* descHeap = tableDescHeap->GetDescriptorHeap().Get();
	commandList->SetDescriptorHeaps(1, &descHeap);

	commandList->ResourceBarrier(1, &barrier);

	D3D12_VIEWPORT vp = { 0, 0, static_cast<FLOAT>(swapChain->Width()), static_cast<FLOAT>(swapChain->Height()) };
	D3D12_RECT rect = CD3DX12_RECT(0, 0, swapChain->Width(), swapChain->Height());

	commandList->RSSetViewports(1, &vp);
	commandList->RSSetScissorRects(1, &rect);

	// Specify the buffers we are going to render to.
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = swapChain->GetBackRTV();
	float lightSteelBlue[4] = { 0.6902, 0.7686, 0.8706, 1.0 };
	commandList->ClearRenderTargetView(backBufferView, lightSteelBlue, 0, nullptr);
	commandList->OMSetRenderTargets(1, &backBufferView, FALSE, nullptr);
}

void CommandQueue::RenderEnd()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChain->GetBackRTVBuffer().Get(),
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
