#pragma once
#include "ZNSwapChain.h"

namespace ZNFramework
{
	class ZNWindow;
	class ZNCommandQueue
	{
	public:
		ZNCommandQueue() = default;
		virtual ~ZNCommandQueue() noexcept = default;

		virtual class ZNSwapChain* CreateSwapChain(const ZNWindow*) = 0;
		//virtual class ZNCommandList* CreateCommandList(const ZNWindow*) = 0;
		
		virtual void Init() = 0;
		virtual void RenderBegin() = 0;
		virtual void RenderEnd() = 0;
		virtual void OnResize(size_t width, size_t height) = 0;
	};
}
