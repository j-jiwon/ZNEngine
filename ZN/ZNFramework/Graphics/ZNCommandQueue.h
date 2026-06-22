#pragma once
#include "ZNSwapChain.h"
#include <functional>

struct ID3D12DescriptorHeap;

namespace ZNFramework
{
	class ZNWindow;
	class ZNSwapChain;
	class ZNCommandQueue
	{
	public:
		ZNCommandQueue() = default;
		virtual ~ZNCommandQueue() noexcept = default;

		virtual void Init(ZNSwapChain* inSwapChain) = 0;
		virtual void RenderBegin() = 0;
		virtual void RenderEnd() = 0;
		virtual void FlushResourceQueue() = 0;
		virtual void WaitSync() = 0;

		virtual float GetGpuFrameTimeMs() const { return 0.0f; }

		void SetForwardRenderCallback(std::function<void()> callback) { forwardRenderCallback = callback; }
		void SetImGuiRenderCallback(std::function<void()> callback) { imguiRenderCallback = callback; }
		void SetImGuiDescriptorHeap(ID3D12DescriptorHeap* heap) { imguiSrvHeap = heap; }

	protected:
		std::function<void()> forwardRenderCallback;
		std::function<void()> imguiRenderCallback;
		ID3D12DescriptorHeap* imguiSrvHeap = nullptr;
	};
}
