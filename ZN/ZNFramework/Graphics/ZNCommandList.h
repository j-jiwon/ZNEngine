#pragma once
namespace ZNFramework
{
	class ZNSwapChain;
	class ZNTexture;
	class ZNCommandList
	{
	public:
		virtual void Reset() = 0;
		virtual void Close() = 0;

		virtual void SetViewport(int width, int height) {};
		virtual void SetScissorRects(int width, int height) {};
	};
}