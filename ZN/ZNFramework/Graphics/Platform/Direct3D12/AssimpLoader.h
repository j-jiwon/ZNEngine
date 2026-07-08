#pragma once
#include "Graphics/ZNModelLoader.h"
#include <unordered_map>
#include <string>

struct aiScene;
struct aiMesh;
struct aiMaterial;

namespace ZNFramework
{
	struct MaterialData;

	using TexIndex = std::unordered_map<std::string, std::filesystem::path>;

	class AssimpLoader : public ZNModelLoader
	{
	public:
		AssimpLoader() = default;
		~AssimpLoader() = default;

		bool Load(const std::filesystem::path& filePath, ModelData& outModelData) override;

	private:
		void ProcessMesh(aiMesh* mesh, const aiScene* scene, ModelData& outModelData);
		void ProcessMaterial(aiMaterial* material, const aiScene* scene,
		                     const std::filesystem::path& modelDir,
		                     const TexIndex& texIndex,
		                     MaterialData& outMaterial);
	};
}
