#pragma once

namespace ZNFramework
{
	class ZNRootSignature
	{
	public:
		ZNRootSignature() = default;
		virtual ~ZNRootSignature() noexcept = default;

		virtual void Init() = 0;
	};
}
