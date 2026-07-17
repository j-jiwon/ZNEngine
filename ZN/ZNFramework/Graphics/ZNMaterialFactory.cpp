#include "ZNMaterialFactory.h"
#include "ZNMaterial.h"
#include "Platform/GraphicsAPI.h"
#include "ZNFramework.h"
#include <filesystem>
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

		// texturePaths are absolute paths (set by AssimpLoader via modelDir / path);
		// embeddedTextureData holds in-memory bytes for GLB-embedded textures.
		for (size_t i = 0; i < static_cast<size_t>(TextureType::Count); ++i)
		{
			ZNTexture* tex = nullptr;

			if (!matData.embeddedTextureData[i].empty())
			{
				tex = Platform::CreateTexture();
				tex->InitFromMemory(matData.embeddedTextureData[i].data(), matData.embeddedTextureData[i].size());
			}
			else if (!matData.texturePaths[i].empty())
			{
				if (!std::filesystem::exists(matData.texturePaths[i]))
				{
					ZNLOG_WARN(LogChannel::Asset, "Texture not found: %s",
						std::filesystem::path(matData.texturePaths[i]).string().c_str());
					continue;
				}
				tex = Platform::CreateTexture();
				tex->Init(matData.texturePaths[i]);
			}

			if (tex)
				material->SetTexture(static_cast<TextureType>(i), tex);
		}

		return material;
	}
}
