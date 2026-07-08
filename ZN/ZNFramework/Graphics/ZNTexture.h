#pragma once
#include "ZNFramework.h"

namespace ZNFramework
{
	class ZNTexture
	{
	public:
		ZNTexture() = default;
		~ZNTexture() = default;

		virtual void Init(const std::wstring& path) = 0;
		// Decodes an in-memory compressed image (e.g. embedded glTF/GLB texture)
		virtual void InitFromMemory(const void* data, size_t size) = 0;
	};
}
