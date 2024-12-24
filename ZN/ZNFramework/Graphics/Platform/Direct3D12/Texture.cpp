#include "Texture.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNTexture* CreateTexture()
	{
		return new Texture();
	}
}

void Texture::SetTextureName()
{
}
