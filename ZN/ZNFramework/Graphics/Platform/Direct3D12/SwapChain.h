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
		SwapChain(GraphicsDevice*, CommandQueue*, const ZNWindow*);
		~SwapChain() noexcept = default;

		uint32_t Width() override { return width; }
		uint32_t Height() override { return height; }

		void Resize(uint32_t width, uint32_t height) override;

	private:
		uint32_t width;
		uint32_t height;

		UINT currentColorTextureIndex;
		mutable ComPtr<ID3D12Resource> colorTexture[FRAME_BUFFER_COUNT];
		mutable ComPtr<ID3D12Resource> depthStencilTexture;

		ComPtr<IDXGISwapChain3> swapChain;

		GraphicsDevice* device;
	};
}
