#pragma once
#include "ZNUtils.h"

namespace ZNFramework
{
	class DescriptorHeap
	{
	public:
		void Init(class GraphicsDevice* inDevice, class SwapChain* inSwapChain);
		void OnResize(size_t width, size_t height);

		D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(int index) { return rtvHandle[index]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferView();

	private:
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		int rtvHeapSize = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[SWAP_CHAIN_BUFFER_COUNT];
		class SwapChain* swapChain;
		class GraphicsDevice* device;
	};
}
