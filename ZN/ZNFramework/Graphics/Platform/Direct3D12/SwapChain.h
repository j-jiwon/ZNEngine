#pragma once
#include "../../ZNSwapChain.h"
#include "ZNUtils.h"
#include "GraphicsDevice.h"

namespace ZNFramework
{
	class ZNWindow;
	class CommandQueue;
	class Texture;

	class SwapChain : public ZNSwapChain
	{
	public:
		SwapChain(GraphicsDevice*, CommandQueue*, const ZNWindow*);
		~SwapChain() noexcept = default;

		void Init() override;

		void Present();
		void SwapIndex();

		ComPtr<IDXGISwapChain3> GetSwapChain() { return swapChain; }
		ComPtr<ID3D12Resource> GetRenderTarget(int index) { return renderTargets[index]; }

		UINT GetCurrentBackBufferIndex() { return backBufferIndex; }
		ComPtr<ID3D12Resource> GetCurrentBackBufferResource() { return renderTargets[backBufferIndex]; }

		uint32_t Width() override { return width; }
		uint32_t Height() override { return height; }

		void Resize(uint32_t inWidth, uint32_t inHeight) override;

		uint32_t width;
		uint32_t height;

		ComPtr<IDXGISwapChain3> swapChain;
		ComPtr<ID3D12Resource> renderTargets[SWAP_CHAIN_BUFFER_COUNT];
		UINT backBufferIndex = 0;

		GraphicsDevice* device;
		CommandQueue* queue;
		HWND window1;
	};
}
