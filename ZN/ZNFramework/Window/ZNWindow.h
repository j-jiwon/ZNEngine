#pragma once
#include "../../ZN.h"
namespace ZNFramework
{
	class ZNWindow
	{
	public:
		virtual void Create() = 0;
		virtual void Destroy() = 0;
		virtual void Show() = 0;
		virtual void Hide() = 0;

		virtual uint32_t Width() const = 0;
		virtual uint32_t Height() const = 0;
		virtual void* PlatformHandle() const = 0;
	};
}
