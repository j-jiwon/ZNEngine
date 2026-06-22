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

		virtual float GetGpuMemoryUsageMB() const { return 0.0f; }
		virtual float GetGpuMemoryBudgetMB() const { return 0.0f; }
	};
}
