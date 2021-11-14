#include "gtest/gtest.h"
#include "ZNFramework.h"

using namespace ZNFramework;

TEST(Window, Create)
{
	try
	{
		auto window = new ZNWindow();
		window->Create();

		EXPECT_EQ(true, true);
	}
	catch(...)
	{
		EXPECT_EQ(true, false);
	}
}
