#pragma once
#include "../../ZN.h"

namespace ZNFramework
{
	class ZNSwapChain
	{
	public:
		enum { FRAME_BUFFER_COUNT = 2 };
		ZNSwapChain() = default;
		~ZNSwapChain() = default;

		virtual uint32_t Width() = 0;
		virtual uint32_t Height() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
	};
}
