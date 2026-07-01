#include "ZNMaterialFactory.h"
#include "ZNMaterial.h"
#include "Platform/GraphicsAPI.h"
#include <iostream>

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

	ZNMaterial* ZNMaterialFactory::CreatePBRFromData(ZNShader* shader, const MaterialData& matData)
	{
		ZNMaterial* material = Platform::CreateMaterial();
		material->Init();
		material->SetShader(shader);
		material->SetParams(matData.params);

		// texturePaths are absolute paths (set by AssimpLoader via modelDir / path)
		for (size_t i = 0; i < static_cast<size_t>(TextureType::Count); ++i)
		{
			const std::wstring& path = matData.texturePaths[i];
			if (path.empty()) continue;
			if (!std::filesystem::exists(path))
			{
				std::cout << "[ZNMaterialFactory] Texture not found: "
				          << std::filesystem::path(path).string() << "\n";
				continue;
			}
			ZNTexture* tex = Platform::CreateTexture();
			tex->Init(path);
			material->SetTexture(static_cast<TextureType>(i), tex);
		}

		return material;
	}
}
