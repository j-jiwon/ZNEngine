#pragma once
#include "ZNApplication.h"

namespace ZNFramework
{
	class ZNApplicationContextInterface
	{
	public:
		ZNApplicationContextInterface() {};
		virtual ~ZNApplicationContextInterface() {};

		virtual int MessageLoop() = 0;
	};
}