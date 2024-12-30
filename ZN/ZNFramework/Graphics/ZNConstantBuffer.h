#pragma once
#include "ZNFramework.h"

namespace ZNFramework
{
	class ZNConstantBuffer
	{
	public:
		ZNConstantBuffer() = default;
		~ZNConstantBuffer() = default;

		virtual void Init(uint32 inSize, uint32 inCount) = 0;
	};
}
