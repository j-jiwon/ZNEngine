#pragma once
#include "../ZNApplicationContextInterface.h"

ZNFramework::ZNApplicationContextInterface* CreateContext()
{
	return new AppicationContext();
}