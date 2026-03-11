#include "DirectionalLight.h"
#include "ZNFramework.h"

using namespace ZNFramework;
using namespace ZNFramework::Platform::Direct3D;

namespace ZNFramework::Platform::Direct3D
{
	ZNDirectionalLight* CreateDirectionalLight()
	{
		return new DirectionalLight();
	}
}

DirectionalLight::DirectionalLight()
{
	// Default directional light pointing down and slightly forward
	direction = ZNVector3(0.5f, -1.0f, 0.5f);
	intensity = 1.0f;
	color = ZNVector3(1.0f, 1.0f, 1.0f);
	ambientIntensity = 0.3f;
}

LightData DirectionalLight::GetLightData() const
{
	LightData data;
	data.lightType = 0; // Directional light
	data.direction = direction;
	data.intensity = intensity;
	data.color = color;
	data.ambientIntensity = ambientIntensity;
	// cameraPos will be set by the renderer
	data.cameraPos = ZNVector3(0.0f, 0.0f, 0.0f);
	return data;
}
