#include "DescriptorHeap.h"
#include "GraphicsDevice.h"
#include "SwapChain.h"

using namespace ZNFramework;

void DescriptorHeap::Init(GraphicsDevice* inDevice, SwapChain* inSwapChain)
{
	device = inDevice;
	swapChain = inSwapChain;
	rtvHeapSize = inDevice->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDesc.NodeMask = 0;

	inDevice->Device()->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap));
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		rtvHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * rtvHeapSize);
		inDevice->Device()->CreateRenderTargetView(inSwapChain->GetRenderTarget(i).Get(), nullptr, rtvHandle[i]);
	}
}

void ZNFramework::DescriptorHeap::OnResize(size_t width, size_t height)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle.Offset(i, rtvHeapSize);
		device->Device()->CreateRenderTargetView(swapChain->GetRenderTarget(i).Get(), nullptr, rtvHandle);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetBackBufferView()
{
	return GetRTV(swapChain->GetCurrentBackBufferIndex());
}

