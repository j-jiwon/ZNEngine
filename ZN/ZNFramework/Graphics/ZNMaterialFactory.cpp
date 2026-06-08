#include "ZNMaterialFactory.h"
#include "ZNMaterial.h"
#include "Platform/GraphicsAPI.h"

namespace ZNFramework
{
	ZNMaterial* ZNMaterialFactory::CreatePBR(ZNShader* shader, const ZNVector4& albedoColor,
		float metallic, float roughness, float ao)
	{
		ZNMaterial* material = Platform::CreateMaterial();
		material->Init();
		material->SetShader(shader);

		MaterialParams params;
		params.albedoColor = albedoColor;
		params.metallic = metallic;
		params.roughness = roughness;
		params.ao = ao;
		material->SetParams(params);

		return material;
	}
}
