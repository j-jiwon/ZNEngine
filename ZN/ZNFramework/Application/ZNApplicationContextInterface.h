#pragma once

namespace ZNFramework
{
	class ZNApplicationContextInterface
	{
	public:
		ZNApplicationContextInterface() = default;
		~ZNApplicationContextInterface() = default;

		virtual int MessageLoop() = 0;
	};
}