#pragma once
#include <string>

namespace ZNFramework
{
	class ZNShader
	{
	public:
		ZNShader() = default;
		virtual ~ZNShader() = default;

		virtual void Load(const std::wstring& filepath) = 0;
		virtual void Bind() = 0;
		virtual void EnableAlphaBlend() {}
	};
}
