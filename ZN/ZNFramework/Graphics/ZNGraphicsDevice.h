#pragma once
#include "ZNCommandQueue.h"
#include "ZNCommandList.h"

namespace ZNFramework
{
	class ZNCommandQueue;
	class ZNCommandList;
	class ZNGraphicsDevice
	{
	public:
		ZNGraphicsDevice() = default;
		virtual ~ZNGraphicsDevice() noexcept = default;

		virtual ZNCommandQueue* CreateCommandQueue() = 0;
		virtual ZNCommandList* CreateCommandList() = 0;

		inline static ZNGraphicsDevice* CreateGraphicsDevice();
	};
}
