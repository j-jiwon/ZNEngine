#pragma once

namespace ZNFramework
{
	class ZNCommandQueue;
	class ZNSwapChain;
	class ZNGraphicsDevice
	{
	public:
		ZNGraphicsDevice() = default;
		virtual ~ZNGraphicsDevice() noexcept = default;

		inline static ZNGraphicsDevice* CreateGraphicsDevice();
	};
}
