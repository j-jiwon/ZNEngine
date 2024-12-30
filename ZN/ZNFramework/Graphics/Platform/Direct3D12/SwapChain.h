#pragma once
#include "Graphics/ZNSwapChain.h"
#include "ZNUtils.h"

namespace ZNFramework
{
	class ZNWindow;
	class CommandQueue;
	class GraphicsDevice;
	class SwapChain : public ZNSwapChain
	{
	public:
		SwapChain() = default;
		~SwapChain() noexcept = default;

		void Init(class ZNCommandQueue* inQueue) override;
		void Resize(uint32 inWidth, uint32 inHeight) override;

		ComPtr<IDXGISwapChain3> GetSwapChain() { return swapChain; }
		ComPtr<ID3D12Resource> GetRenderTarget(int index) { return rtvBuffer[index]; }

		UINT GetCurrentBackBufferIndex() { return backBufferIndex; }
		ComPtr<ID3D12Resource> GetBackRTVBuffer() { return rtvBuffer[backBufferIndex]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetBackRTV() { return rtvHandle[backBufferIndex]; }

		uint32 Width() override { return width; }
		uint32 Height() override { return height; }

		void Present();
		void SwapIndex();

	private:
		void CreateSwapChainInternal();
		void CreateRTV();

	private:
		ComPtr<IDXGISwapChain3> swapChain;
		ComPtr<ID3D12Resource> rtvBuffer[SWAP_CHAIN_BUFFER_COUNT];
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[SWAP_CHAIN_BUFFER_COUNT];
		uint32 backBufferIndex = 0;
		uint32 rtvHeapSize;

		GraphicsDevice* device;
		CommandQueue* queue;
		HWND hwnd;
		uint32 width;
		uint32 height;
	};
}
