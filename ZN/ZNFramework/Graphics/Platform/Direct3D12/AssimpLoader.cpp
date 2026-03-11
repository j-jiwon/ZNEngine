#include "AssimpLoader.h"
#include "ZNFramework.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNModelLoader* CreateModelLoader()
	{
		return new AssimpLoader();
	}
}

bool AssimpLoader::Load(const std::filesystem::path& filePath, ModelData& outModelData)
{
	Assimp::Importer importer;

	// Load scene with common post-processing flags
	const aiScene* scene = importer.ReadFile(
		filePath.string(),
		aiProcess_Triangulate |           // Convert all polygons to triangles
		aiProcess_GenNormals |            // Generate normals if not present
		aiProcess_FlipUVs |               // Flip UVs for DirectX
		aiProcess_CalcTangentSpace |      // Calculate tangent space for normal mapping
		aiProcess_JoinIdenticalVertices | // Optimize by joining identical vertices
		aiProcess_SortByPType             // Split meshes by primitive type
	);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		// Failed to load
		return false;
	}

	// Get model directory for resolving texture paths
	std::filesystem::path modelDir = filePath.parent_path();

	// Reserve space for materials
	outModelData.materials.resize(scene->mNumMaterials);

	// Process materials first
	for (uint32 i = 0; i < scene->mNumMaterials; ++i)
	{
		ProcessMaterial(scene->mMaterials[i], scene, modelDir, outModelData.materials[i]);
	}

	// Process all meshes
	for (uint32 i = 0; i < scene->mNumMeshes; ++i)
	{
		ProcessMesh(scene->mMeshes[i], scene, outModelData);
	}

	return true;
}

void AssimpLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, ModelData& outModelData)
{
	MeshData meshData;

	// Reserve space for vertices and indices
	meshData.vertices.reserve(mesh->mNumVertices);
	meshData.indices.reserve(mesh->mNumFaces * 3);

	// Extract vertices
	for (uint32 i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex;

		// Position
		vertex.pos = ZNVector3(
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z
		);

		// Color (default white if not present)
		if (mesh->mColors[0])
		{
			vertex.color = ZNVector4(
				mesh->mColors[0][i].r,
				mesh->mColors[0][i].g,
				mesh->mColors[0][i].b,
				mesh->mColors[0][i].a
			);
		}
		else
		{
			vertex.color = ZNVector4(1.f, 1.f, 1.f, 1.f);
		}

		// UV coordinates (use first UV set)
		if (mesh->mTextureCoords[0])
		{
			vertex.uv = ZNVector2(
				mesh->mTextureCoords[0][i].x,
				mesh->mTextureCoords[0][i].y
			);
		}
		else
		{
			vertex.uv = ZNVector2(0.f, 0.f);
		}

		// Normal (should always be present due to aiProcess_GenNormals flag)
		if (mesh->mNormals)
		{
			vertex.normal = ZNVector3(
				mesh->mNormals[i].x,
				mesh->mNormals[i].y,
				mesh->mNormals[i].z
			);
		}
		else
		{
			vertex.normal = ZNVector3(0.f, 1.f, 0.f); // Default up
		}

		meshData.vertices.push_back(vertex);
	}

	// Extract indices
	for (uint32 i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		for (uint32 j = 0; j < face.mNumIndices; ++j)
		{
			meshData.indices.push_back(face.mIndices[j]);
		}
	}

	// Material index
	meshData.materialIndex = mesh->mMaterialIndex;

	outModelData.meshes.push_back(meshData);
}

void AssimpLoader::ProcessMaterial(aiMaterial* material, const aiScene* scene, const std::filesystem::path& modelDir, MaterialData& outMaterial)
{
	// Get base color / diffuse color
	aiColor4D diffuse;
	if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
	{
		outMaterial.params.albedoColor = ZNVector4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
	}

	// Get metallic value (if available)
	float metallic = 0.0f;
	if (AI_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, metallic))
	{
		outMaterial.params.metallic = metallic;
	}

	// Get roughness value (if available)
	float roughness = 0.5f;
	if (AI_SUCCESS == material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
	{
		outMaterial.params.roughness = roughness;
	}

	// Load texture paths
	auto LoadTexturePath = [&](aiTextureType type, TextureType textureType)
	{
		if (material->GetTextureCount(type) > 0)
		{
			aiString path;
			material->GetTexture(type, 0, &path);

			// Convert to absolute path
			std::filesystem::path texPath = modelDir / path.C_Str();
			outMaterial.texturePaths[static_cast<size_t>(textureType)] = texPath.wstring();
		}
	};

	LoadTexturePath(aiTextureType_DIFFUSE, TextureType::Albedo);
	LoadTexturePath(aiTextureType_NORMALS, TextureType::Normal);
	LoadTexturePath(aiTextureType_METALNESS, TextureType::Metallic);
	LoadTexturePath(aiTextureType_DIFFUSE_ROUGHNESS, TextureType::Roughness);
	LoadTexturePath(aiTextureType_AMBIENT_OCCLUSION, TextureType::AO);
}
