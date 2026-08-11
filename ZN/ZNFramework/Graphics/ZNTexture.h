#pragma once
#include "ZNFramework.h"

namespace ZNFramework
{
	class ZNTexture
	{
	public:
		ZNTexture() = default;
		virtual ~ZNTexture() = default;

		virtual void Init(const std::wstring& path, bool srgb) = 0;
		// Decodes an in-memory compressed image (e.g. embedded glTF/GLB texture)
		virtual void InitFromMemory(const void* data, size_t size, bool srgb) = 0;
		// Procedural 1x1 solid-color texture (used as a deterministic "no texture" placeholder)
		virtual void InitSolidColor(uint8 r, uint8 g, uint8 b, uint8 a) = 0;
	};
}
