#pragma once
namespace ZNFramework
{
	class ZNTexture
	{
		virtual void SetTextureName() = 0; 

		inline static ZNTexture* CreateTexture();
	};
}
