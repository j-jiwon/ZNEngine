#pragma once
#ifdef _WIN32
#include "../../ZNApplicationContextInterface.h"

namespace ZNFramework
{
	class ApplicationContext : public ZNApplicationContextInterface
	{
	public:
		ApplicationContext() {};
		~ApplicationContext() {};
		int MessageLoop() override;
	private:
		int exitCode = 0;
	};
}

#endif