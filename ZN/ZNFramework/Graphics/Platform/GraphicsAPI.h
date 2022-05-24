#pragma once
#include "../ZNGraphicsDevice.h"

namespace ZNFramework::Platform
{
#if defined(_WIN32)
	namespace Direct3D
	{
		ZNGraphicsDevice* CreateGraphicsDevice();
	}
#endif

	ZNGraphicsDevice* CreateGraphicsDevice()
	{
		return Direct3D::CreateGraphicsDevice();
	}
}
