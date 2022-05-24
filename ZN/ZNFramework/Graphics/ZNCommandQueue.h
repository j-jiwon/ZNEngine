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

		virtual ZNSwapChain* CreateSwapChain(const ZNWindow*) = 0;
	};
}
