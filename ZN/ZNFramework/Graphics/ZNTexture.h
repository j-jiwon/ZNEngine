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
	};
}
