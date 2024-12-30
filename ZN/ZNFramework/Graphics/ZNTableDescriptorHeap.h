#pragma once
#include "ZNFramework.h"

namespace ZNFramework
{
	class ZNTableDescriptorHeap
	{
	public:
		ZNTableDescriptorHeap() = default;
		~ZNTableDescriptorHeap() = default;

		virtual void Init(uint32 inCount) = 0;
	};
}
