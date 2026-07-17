#pragma once
#include "Graphics/ZNSwapChain.h"
#include "ZNUtils.h"
#include <dxgi1_4.h> // IDXGISwapChain3::GetCurrentBackBufferIndex

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

		ComPtr<IDXGISwapChain> GetSwapChain() { return swapChain; }
		ComPtr<ID3D12Resource> GetRenderTarget(int index) { return rtvBuffer[index]; }

		// Query DXGI for the live back-buffer index every call instead of a manual counter — with
		// the flip model the two can drift apart (e.g. across ResizeBuffers), which would make us
		// render into a buffer that isn't the one being presented (a stale-looking screen).
		UINT GetCurrentBackBufferIndex() { return swapChain->GetCurrentBackBufferIndex(); }
		ComPtr<ID3D12Resource> GetBackRTVBuffer() { return rtvBuffer[GetCurrentBackBufferIndex()]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetBackRTV() { return rtvHandle[GetCurrentBackBufferIndex()]; }

		uint32 Width() override { return width; }
		uint32 Height() override { return height; }

		void Present();

	private:
		void CreateSwapChainInternal();
		void CreateRTV();

	private:
		ComPtr<IDXGISwapChain3>	swapChain;
		ComPtr<ID3D12Resource> rtvBuffer[SWAP_CHAIN_BUFFER_COUNT];
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[SWAP_CHAIN_BUFFER_COUNT];
		uint32 rtvHeapSize;

		GraphicsDevice* device;
		CommandQueue* queue;
		HWND hwnd;
		uint32 width;
		uint32 height;
	};
}
