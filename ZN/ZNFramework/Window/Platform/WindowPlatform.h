#pragma once
#include "Win32/Window.h"

using namespace ZNFramework;

namespace ZNFramework
{
	class WindowPlatform
	{
	public:
		static ZNWindow* Create()
		{
			return new Window();
		};
	};
}
