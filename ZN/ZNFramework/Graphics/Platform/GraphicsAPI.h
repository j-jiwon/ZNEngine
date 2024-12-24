#pragma once
#include "../ZNGraphicsDevice.h"
#include "../ZNTexture.h"

namespace ZNFramework::Platform
{
#if defined(_WIN32)
	namespace Direct3D
	{
		ZNGraphicsDevice* CreateGraphicsDevice();

		ZNTexture* CreateTexture();
	}
#endif

	inline ZNGraphicsDevice* CreateGraphicsDevice()
	{
		return Direct3D::CreateGraphicsDevice();
	}

	inline ZNTexture* CreateTexture()
	{
		return Direct3D::CreateTexture();
	}
}
