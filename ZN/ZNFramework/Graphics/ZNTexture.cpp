#include "ZNTexture.h"
#include "Platform/GraphicsAPI.h"

using namespace ZNFramework;

ZNTexture* ZNTexture::CreateTexture()
{
    return ZNFramework::Platform::CreateTexture();
}
