#pragma once
#include "ZNSwapChain.h"
#include <functional>

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

		// Forward render callback - called after deferred lighting, before present
		void SetForwardRenderCallback(std::function<void()> callback) { forwardRenderCallback = callback; }

	protected:
		std::function<void()> forwardRenderCallback;
	};
}
