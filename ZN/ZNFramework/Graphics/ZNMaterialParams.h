#pragma once
#include "../Math/ZNVector4.h"

namespace ZNFramework
{
	struct MaterialParams
	{
		ZNVector4 albedoColor = ZNVector4(1.f, 0.f, 0.f, 1.f);
		float metallic  = 0.0f;
		float roughness = 0.5f;
		float ao        = 1.0f;
		float padding   = 0.0f; // 16-byte alignment
	};
}
