#include "AssimpLoader.h"
#include "ZNFramework.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstring>

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
		aiProcess_Triangulate |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType |
		aiProcess_PreTransformVertices |  // Bake node-hierarchy transforms into vertex data
		// Assimp's internal representation is always right-handed; our engine (D3D LookAtLH /
		// perspective) is left-handed. This bundles MakeLeftHanded + FlipWindingOrder + FlipUVs -
		// without it, mirrored/negative-determinant nodes (and technically everything) come in
		// with the wrong handedness, which is invisible on generic geometry but shows up as
		// backwards text/numbers on any readable texture.
		aiProcess_ConvertToLeftHanded
	);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		// Failed to load
		return false;
	}

	// Get model directory for resolving texture paths
	std::filesystem::path modelDir = filePath.parent_path();

	// Build filename→path index by scanning modelDir recursively.
	// Allows resolving textures regardless of whether FBX stores relative or absolute paths.
	TexIndex texIndex;
	std::error_code ec;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(modelDir, ec))
	{
		if (!entry.is_regular_file()) continue;
		std::string name = entry.path().filename().string();
		std::string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		texIndex.emplace(lower, entry.path());
	}

	// Reserve space for materials
	outModelData.materials.resize(scene->mNumMaterials);

	// Process materials first
	for (uint32 i = 0; i < scene->mNumMaterials; ++i)
	{
		ProcessMaterial(scene->mMaterials[i], scene, modelDir, texIndex, outModelData.materials[i]);
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

void AssimpLoader::ProcessMaterial(aiMaterial* material, const aiScene* scene,
	const std::filesystem::path& modelDir, const TexIndex& texIndex, MaterialData& outMaterial)
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

	// Diagnostic: dump every texture type that has at least one entry, showing raw FBX path
	aiString matName;
	material->Get(AI_MATKEY_NAME, matName);
	static const struct { aiTextureType type; const char* name; } kAllTypes[] = {
		{ aiTextureType_DIFFUSE,          "DIFFUSE"          },
		{ aiTextureType_SPECULAR,         "SPECULAR"         },
		{ aiTextureType_AMBIENT,          "AMBIENT"          },
		{ aiTextureType_EMISSIVE,         "EMISSIVE"         },
		{ aiTextureType_HEIGHT,           "HEIGHT"           },
		{ aiTextureType_NORMALS,          "NORMALS"          },
		{ aiTextureType_SHININESS,        "SHININESS"        },
		{ aiTextureType_OPACITY,          "OPACITY"          },
		{ aiTextureType_DISPLACEMENT,     "DISPLACEMENT"     },
		{ aiTextureType_LIGHTMAP,         "LIGHTMAP"         },
		{ aiTextureType_REFLECTION,       "REFLECTION"       },
		{ aiTextureType_BASE_COLOR,       "BASE_COLOR"       },
		{ aiTextureType_NORMAL_CAMERA,    "NORMAL_CAMERA"    },
		{ aiTextureType_EMISSION_COLOR,   "EMISSION_COLOR"   },
		{ aiTextureType_METALNESS,        "METALNESS"        },
		{ aiTextureType_DIFFUSE_ROUGHNESS,"DIFFUSE_ROUGHNESS"},
		{ aiTextureType_AMBIENT_OCCLUSION,"AMBIENT_OCCLUSION"},
		{ aiTextureType_GLTF_METALLIC_ROUGHNESS, "GLTF_METALLIC_ROUGHNESS" },
		{ aiTextureType_UNKNOWN,          "UNKNOWN"          },
	};
	bool anyTex = false;
	for (const auto& entry : kAllTypes)
	{
		unsigned int cnt = material->GetTextureCount(entry.type);
		if (cnt == 0) continue;
		anyTex = true;
		for (unsigned int ti = 0; ti < cnt; ++ti)
		{
			aiString rawPath;
			material->GetTexture(entry.type, ti, &rawPath);
			ZNLOG_TRACE(LogChannel::Asset, "  %s [%s] raw=\"%s\"",
			            matName.C_Str(), entry.name, rawPath.C_Str());
		}
	}
	if (!anyTex)
		ZNLOG_TRACE(LogChannel::Asset, "%s: no textures in FBX", matName.C_Str());

	// Fallback for texture types GetTextureCount()/GetTexture() fail to find even though
	// the material actually has the property (observed with this Assimp build for the
	// newer PBR slots — BASE_COLOR/METALNESS/DIFFUSE_ROUGHNESS/GLTF_METALLIC_ROUGHNESS —
	// while classic slots like DIFFUSE/NORMALS/EMISSIVE work fine via the normal API).
	// Reads the "$tex.file" property's serialized aiString directly: a 4-byte length
	// prefix followed by that many chars (assimp's standard string-property encoding).
	auto GetRawTexturePath = [&](aiTextureType type, unsigned int index) -> std::string
	{
		for (unsigned int pi = 0; pi < material->mNumProperties; ++pi)
		{
			const aiMaterialProperty* prop = material->mProperties[pi];
			if (prop->mSemantic != static_cast<unsigned int>(type) || prop->mIndex != index) continue;
			if (std::strcmp(prop->mKey.C_Str(), "$tex.file") != 0) continue;
			if (prop->mType != aiPTI_String) continue;
			if (prop->mDataLength < sizeof(uint32)) continue;
			uint32 len = 0;
			std::memcpy(&len, prop->mData, sizeof(uint32));
			if (sizeof(uint32) + len > prop->mDataLength) continue;
			return std::string(prop->mData + sizeof(uint32), len);
		}
		return {};
	};

	// Resolve by: (1) modelDir / rawPath as-is, (2) filename lookup in pre-built index.
	// The index covers the full modelDir subtree, so absolute paths on other machines
	// (e.g. "G:\...\texture.jpg") are resolved by extracting just the filename.
	auto ResolveTexPath = [&](const char* rawCStr) -> std::wstring
	{
		std::filesystem::path raw(rawCStr);
		// 1. Try raw path relative to modelDir (handles "../subdir/file" cases)
		std::filesystem::path candidate = modelDir / raw;
		if (std::filesystem::exists(candidate)) return candidate.wstring();
		// 2. Filename-only lookup in texIndex (handles absolute paths from other machines)
		std::string name = raw.filename().string();
		std::string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		auto it = texIndex.find(lower);
		if (it != texIndex.end()) return it->second.wstring();
		return {};
	};

	// Tries each candidate aiTextureType in order; first one with actual data wins.
	// Embedded textures (e.g. GLB, where the raw path looks like "*0" and indexes
	// scene->mTextures) are captured as in-memory bytes; everything else is resolved
	// to a file on disk via ResolveTexPath.
	auto LoadTexturePath = [&](std::initializer_list<aiTextureType> types, TextureType slot)
	{
		for (aiTextureType type : types)
		{
			std::string rawStr;
			if (material->GetTextureCount(type) > 0)
			{
				aiString rawPath;
				material->GetTexture(type, 0, &rawPath);
				rawStr = rawPath.C_Str();
			}
			else
			{
				rawStr = GetRawTexturePath(type, 0);
				if (rawStr.empty()) continue;
			}

			if (const aiTexture* embedded = scene->GetEmbeddedTexture(rawStr.c_str()))
			{
				if (embedded->mHeight != 0)
					continue; // uncompressed embedded texel data — not supported, try next candidate

				const uint8* bytes = reinterpret_cast<const uint8*>(embedded->pcData);
				outMaterial.embeddedTextureData[static_cast<size_t>(slot)]
					.assign(bytes, bytes + embedded->mWidth);
				return;
			}

			std::wstring resolved = ResolveTexPath(rawStr.c_str());
			if (!resolved.empty())
			{
				outMaterial.texturePaths[static_cast<size_t>(slot)] = resolved;
				return;
			}
		}
	};

	// Albedo: aiTextureType_BASE_COLOR is glTF's PBR slot; DIFFUSE covers FBX/OBJ.
	LoadTexturePath({ aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE }, TextureType::Albedo);

	// Normal
	LoadTexturePath({ aiTextureType_NORMAL_CAMERA, aiTextureType_NORMALS, aiTextureType_HEIGHT }, TextureType::Normal);

	// ARM (AO/Roughness/Metallic). glTF's combined metallicRoughness texture
	// (G=roughness, B=metallic, R usually unused/AO) maps directly to this layout;
	// classic FBX sometimes stores roughness as SHININESS.
	LoadTexturePath({ aiTextureType_GLTF_METALLIC_ROUGHNESS, aiTextureType_UNKNOWN,
	                  aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_SHININESS,
	                  aiTextureType_AMBIENT_OCCLUSION }, TextureType::ARM);

	// Emissive. EMISSION_COLOR is glTF's slot; EMISSIVE covers FBX/OBJ.
	LoadTexturePath({ aiTextureType_EMISSION_COLOR, aiTextureType_EMISSIVE }, TextureType::Emissive);

	// glTF emissiveFactor, scaling the texture above. Spec default is black; when a model has an
	// emissive texture but no readable factor, fall back to white (importer gap, not intent).
	{
		bool hasEmissiveTex =
			!outMaterial.texturePaths[static_cast<size_t>(TextureType::Emissive)].empty() ||
			!outMaterial.embeddedTextureData[static_cast<size_t>(TextureType::Emissive)].empty();

		aiColor4D emissive;
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive))
			outMaterial.params.emissiveColor = ZNVector4(emissive.r, emissive.g, emissive.b, 1.f);
		else if (hasEmissiveTex)
			outMaterial.params.emissiveColor = ZNVector4(1.f, 1.f, 1.f, 1.f);
	}

	// Log resolved results
	const char* slotNames[] = { " Albedo=", " Normal=", " ARM=", " Emissive=" };
	bool anyResolved = false;
	for (size_t i = 0; i < static_cast<size_t>(TextureType::Count); ++i)
		if (!outMaterial.texturePaths[i].empty() || !outMaterial.embeddedTextureData[i].empty())
			{ anyResolved = true; break; }
	if (anyResolved)
	{
		std::string dump = std::string("Resolved ") + matName.C_Str();
		for (size_t i = 0; i < static_cast<size_t>(TextureType::Count); ++i)
		{
			dump += slotNames[i];
			if (!outMaterial.embeddedTextureData[i].empty())
				dump += "(embedded, " + std::to_string(outMaterial.embeddedTextureData[i].size()) + " bytes)";
			else if (!outMaterial.texturePaths[i].empty())
				dump += std::filesystem::path(outMaterial.texturePaths[i]).filename().string();
			else
				dump += "(none)";
		}
		ZNLOG_TRACE(LogChannel::Asset, "%s", dump.c_str());
	}
}
