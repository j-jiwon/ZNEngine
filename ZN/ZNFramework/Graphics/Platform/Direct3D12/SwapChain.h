#pragma once
#include "../../ZNSwapChain.h"
#include "ZNUtils.h"
#include "GraphicsDevice.h"

namespace ZNFramework
{
	class ZNWindow;
	class CommandQueue;

	class SwapChain : public ZNSwapChain
	{
	public:
		SwapChain() = default;
		~SwapChain() noexcept = default;

		void Init(class ZNCommandQueue* inQueue) override;
		void Resize(uint32_t inWidth, uint32_t inHeight) override;

		ComPtr<IDXGISwapChain3> GetSwapChain() { return swapChain; }
		ComPtr<ID3D12Resource> GetRenderTarget(int index) { return rtvBuffer[index]; }

		UINT GetCurrentBackBufferIndex() { return backBufferIndex; }
		ComPtr<ID3D12Resource> GetBackRTVBuffer() { return rtvBuffer[backBufferIndex]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetBackRTV() { return rtvHandle[backBufferIndex]; }

		uint32_t Width() override { return width; }
		uint32_t Height() override { return height; }

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
		UINT backBufferIndex = 0;
		int rtvHeapSize;

		GraphicsDevice* device;
		CommandQueue* queue;
		HWND hwnd;
		uint32_t width;
		uint32_t height;
	};
}
