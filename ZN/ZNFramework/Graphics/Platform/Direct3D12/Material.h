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
		~Material() override;
		void Init() override;
		void SetShader(ZNShader* shader) override;
		void SetTexture(TextureType type, ZNTexture* texture) override;
		void CopyTexturesFrom(const ZNMaterial* other) override;
		void SetParams(const MaterialParams& params) override;
		const MaterialParams& GetParams() const override { return params; }
		void Bind() override;

		// Override t0 (albedo) slot with an externally managed SRV — e.g. a RenderTexture.
		// The caller must keep the descriptor heap alive for the lifetime of this material.
		void SetAlbedoSRVHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
			albedoSRVOverride    = handle;
			hasAlbedoSRVOverride = true;
		}
		void ClearAlbedoSRVHandle() { hasAlbedoSRVOverride = false; }

	private:
		ZNShader* shader = nullptr;
		std::array<Texture*, static_cast<size_t>(TextureType::Count)> textures = {};
		std::array<bool,     static_cast<size_t>(TextureType::Count)> ownsTexture = {};
		MaterialParams params;

		D3D12_CPU_DESCRIPTOR_HANDLE albedoSRVOverride   = {};
		bool                         hasAlbedoSRVOverride = false;
	};
}
