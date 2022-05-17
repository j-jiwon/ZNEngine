#pragma once
#ifdef _WIN32
#include "../../ZNApplicationContextInterface.h"

namespace ZNFramework
{
	class ApplicationContext : public ZNApplicationContextInterface
	{
	public:
		ApplicationContext() {};
		// ApplicationContext(Window window, GraphicDevice device);
		int MessageLoop() override;
	private:
		// TODO: window, graphicdevice, handle 
	};
}

#endif