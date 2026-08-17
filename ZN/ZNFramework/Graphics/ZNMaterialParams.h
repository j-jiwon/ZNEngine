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
		float useAlbedoTexture = 0.0f; // 1.0 = sample t0; 0.0 = use albedoColor only

		// glTF emissiveFactor; multiplies the emissive texture (t2 slot's neighbour, t4).
		// 16-byte aligned at offset 32 to match HLSL's cbMaterial packing.
		ZNVector4 emissiveColor = ZNVector4(0.f, 0.f, 0.f, 0.f);

		// Multiply the final roughness/metallic, whichever source produced them. An ARM texture
		// replaces the two scalars above (see gbuffer.hlsli), so these are how a textured
		// material is tuned — scaling keeps the map's own variation. 1.0 = unchanged.
		float roughnessScale = 1.0f;
		float metallicScale  = 1.0f;
		float useARMTexture  = 0.0f;
	};
}
