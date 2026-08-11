#include "ZNMaterialFactory.h"
#include "ZNMaterial.h"
#include "Platform/GraphicsAPI.h"
#include "ZNFramework.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace ZNFramework
{
	ZNMaterial* ZNMaterialFactory::CreatePBR(ZNShader* shader, const ZNVector4& albedoColor,
		float metallic, float roughness, float ao)
	{
		std::unique_ptr<ZNMaterial> material(Platform::CreateMaterial());
		material->Init();
		material->SetShader(shader);

		MaterialParams params;
		params.albedoColor = albedoColor;
		params.metallic = metallic;
		params.roughness = roughness;
		params.ao = ao;
		material->SetParams(params);

		return material.release();
	}

	ZNMaterial* ZNMaterialFactory::CreatePBRFromData(ZNShader* shader, const MaterialData& matData)
	{
		std::unique_ptr<ZNMaterial> material(Platform::CreateMaterial());
		material->Init();
		material->SetShader(shader);
		material->SetParams(matData.params);

		// texturePaths are absolute paths (set by AssimpLoader via modelDir / path);
		// embeddedTextureData holds in-memory bytes for GLB-embedded textures.
		for (size_t i = 0; i < static_cast<size_t>(TextureType::Count); ++i)
		{
			std::unique_ptr<ZNTexture> tex;
			const bool srgb = (static_cast<TextureType>(i) == TextureType::Albedo);

			if (!matData.embeddedTextureData[i].empty())
			{
				tex.reset(Platform::CreateTexture());
				tex->InitFromMemory(matData.embeddedTextureData[i].data(), matData.embeddedTextureData[i].size(), srgb);
			}
			else if (!matData.texturePaths[i].empty())
			{
				if (!std::filesystem::exists(matData.texturePaths[i]))
				{
					throw std::runtime_error("texture file lookup failed: path=" +
						std::filesystem::path(matData.texturePaths[i]).string());
				}
				tex.reset(Platform::CreateTexture());
				tex->Init(matData.texturePaths[i], srgb);
			}

			if (tex)
				material->SetTexture(static_cast<TextureType>(i), tex.release());
		}

		return material.release();
	}
}
