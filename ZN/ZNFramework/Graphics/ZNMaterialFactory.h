#pragma once

namespace ZNFramework
{
	class ZNMaterial;
	class ZNShader;
	class ZNVector4;
	struct MaterialData;

	class ZNMaterialFactory
	{
	public:
		// Creates a PBR material with the given parameters
		// shader: the shader to use
		// albedoColor: base color (RGBA)
		// metallic: 0.0 (dielectric) to 1.0 (metal)
		// roughness: 0.0 (smooth) to 1.0 (rough)
		// ao: ambient occlusion (default 1.0)
		static ZNMaterial* CreatePBR(ZNShader* shader, const ZNVector4& albedoColor,
			float metallic = 0.0f, float roughness = 0.5f, float ao = 1.0f);

		// Creates a PBR material from a MaterialData struct (params + texture paths)
		static ZNMaterial* CreatePBRFromData(ZNShader* shader, const MaterialData& matData);

	private:
		ZNMaterialFactory() = delete;
	};
}
