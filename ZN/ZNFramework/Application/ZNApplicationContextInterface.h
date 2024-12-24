#pragma once


namespace ZNFramework
{
	class ZNWindow;
	class ZNGraphicsDevice;
	class ZNApplicationContextInterface
	{
	public:
		ZNApplicationContextInterface() = default;
		~ZNApplicationContextInterface() = default;

		virtual void Initialize(ZNWindow* window, ZNGraphicsDevice* device) = 0;
		virtual int MessageLoop() = 0;
	};
}