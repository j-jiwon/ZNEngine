#pragma once
#include "../../ZNInclude.h"
#include "ZNApplicationContextInterface.h"

namespace ZNFramework
{	
	class ZNApplicationContextInterface;
	class ZNApplication
	{
	public:
		ZNApplication();
		~ZNApplication();
		int Run();		// process message

		virtual void OnInitialize() = 0;
		virtual void OnTerminate() = 0;

	protected:
		ZNApplicationContextInterface* context;
	};
}
