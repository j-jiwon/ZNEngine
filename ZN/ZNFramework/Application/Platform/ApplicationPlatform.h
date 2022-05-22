#pragma once
#include "../ZNApplicationContextInterface.h"
#include "Win32/ApplicationContext.h"

ZNFramework::ZNApplicationContextInterface* CreateContext()
{
	return new ZNFramework::ApplicationContext();
}