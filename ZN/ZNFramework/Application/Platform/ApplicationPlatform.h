#pragma once
#include "../ZNApplicationContextInterface.h"
#include "Win32/ApplicationContext.h"

using namespace ZNFramework;

// create context according to platform 
	
ZNFramework::ZNApplicationContextInterface* CreateContext()
{
	return new ApplicationContext();
}
