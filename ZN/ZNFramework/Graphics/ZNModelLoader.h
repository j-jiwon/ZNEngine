#pragma once
#include <string>
#include <filesystem>

namespace ZNFramework
{
	struct ModelData;

	class ZNModelLoader
	{
	public:
		ZNModelLoader() = default;
		virtual ~ZNModelLoader() = default;

		virtual bool Load(const std::filesystem::path& filePath, ModelData& outModelData) = 0;
	};
}
