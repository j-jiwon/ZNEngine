#pragma once
#include "Graphics/ZNModelLoader.h"

struct aiScene;
struct aiMesh;
struct aiMaterial;

namespace ZNFramework
{
	struct MaterialData;

	class AssimpLoader : public ZNModelLoader
	{
	public:
		AssimpLoader() = default;
		~AssimpLoader() = default;

		bool Load(const std::filesystem::path& filePath, ModelData& outModelData) override;

	private:
		void ProcessMesh(aiMesh* mesh, const aiScene* scene, ModelData& outModelData);
		void ProcessMaterial(aiMaterial* material, const aiScene* scene, const std::filesystem::path& modelDir, MaterialData& outMaterial);
	};
}
