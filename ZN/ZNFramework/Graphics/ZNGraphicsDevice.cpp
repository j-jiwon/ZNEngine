#include "ZNGraphicsDevice.h"
#include "Platform/GraphicsAPI.h"

using namespace ZNFramework;

ZNGraphicsDevice* ZNGraphicsDevice::CreateGraphicsDevice()
{
    return ZNFramework::Platform::CreateGraphicsDevice();
}
