#pragma once

namespace ZNFramework
{
	class ZNImGuiLayer
	{
	public:
		virtual ~ZNImGuiLayer() = default;
		virtual void Shutdown() = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
	};
}
