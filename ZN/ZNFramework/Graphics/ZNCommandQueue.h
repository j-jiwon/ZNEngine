#pragma once
#include "ZNSwapChain.h"

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
	};
}
