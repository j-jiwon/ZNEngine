#pragma once
#ifdef _WIN32
#include "../../ZNApplicationContextInterface.h"

namespace ZNFramework
{
	class ApplicationContext : public ZNApplicationContextInterface
	{
	public:
		ApplicationContext() = default;
		~ApplicationContext() = default;
		int MessageLoop() override;
	};
}

#endif