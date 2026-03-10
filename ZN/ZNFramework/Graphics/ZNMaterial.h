#pragma once
#include <string>

namespace ZNFramework
{
	class ZNShader;
	class ZNTexture;
	struct MaterialParams;
	enum class TextureType : unsigned char;

	class ZNMaterial
	{
	public:
		ZNMaterial() = default;
		virtual ~ZNMaterial() = default;

		virtual void Init() = 0;
		virtual void SetShader(ZNShader* shader) = 0;
		virtual void SetTexture(TextureType type, ZNTexture* texture) = 0;
		virtual void SetParams(const MaterialParams& params) = 0;
		virtual void Bind() = 0;
	};
}
