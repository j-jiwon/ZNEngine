#pragma once

namespace ZNFramework
{
	class ZNDepthStencilBuffer
	{
	public:
		ZNDepthStencilBuffer() = default;
		~ZNDepthStencilBuffer() = default;

		virtual void Init() = 0;
	};
}