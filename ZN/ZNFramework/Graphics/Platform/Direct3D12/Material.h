#pragma once
#include "Graphics/ZNMaterial.h"
#include "ZNUtils.h"
#include <array>

namespace ZNFramework
{
	class ZNShader;
	class ZNTexture;
	class Texture;

	class Material : public ZNMaterial
	{
	public:
		void Init() override;
		void SetShader(ZNShader* shader) override;
		void SetTexture(TextureType type, ZNTexture* texture) override;
		void SetParams(const MaterialParams& params) override;
		void Bind() override;

	private:
		ZNShader* shader = nullptr;
		std::array<Texture*, static_cast<size_t>(TextureType::Count)> textures = {};
		MaterialParams params;
	};
}
