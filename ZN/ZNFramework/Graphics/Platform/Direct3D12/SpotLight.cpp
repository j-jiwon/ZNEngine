#include "SpotLight.h"
#include "ZNFramework.h"
#include <cmath>

using namespace ZNFramework;
using namespace ZNFramework::Platform::Direct3D;

namespace ZNFramework::Platform::Direct3D
{
	ZNSpotLight* CreateSpotLight()
	{
		return new SpotLight();
	}
}

SpotLight::SpotLight()
{
	// Default spot light pointing down from above
	position = ZNVector3(0.0f, 5.0f, 0.0f);
	direction = ZNVector3(0.0f, -1.0f, 0.0f);
	intensity = 1.0f;
	color = ZNVector3(1.0f, 1.0f, 1.0f);
	ambientIntensity = 0.1f;
	innerCutoffAngle = 12.5f;
	outerCutoffAngle = 17.5f;
	constantAttenuation = 1.0f;
	linearAttenuation = 0.09f;
	quadraticAttenuation = 0.032f;
}

LightData SpotLight::GetLightData() const
{
	LightData data;
	data.lightType = 2; // Spot light
	data.position = position;
	data.direction = direction;
	data.intensity = intensity;
	data.color = color;
	data.ambientIntensity = ambientIntensity;

	// Convert angles from degrees to cosine for shader
	const float PI = 3.14159265359f;
	data.cutoffAngle = std::cos(innerCutoffAngle * PI / 180.0f);
	data.outerCutoffAngle = std::cos(outerCutoffAngle * PI / 180.0f);

	data.constant = constantAttenuation;
	data.linear = linearAttenuation;
	data.quadratic = quadraticAttenuation;

	// cameraPos will be set by the renderer
	data.cameraPos = ZNVector3(0.0f, 0.0f, 0.0f);

	return data;
}

void SpotLight::SetCutoffAngle(float innerAngle, float outerAngle)
{
	innerCutoffAngle = innerAngle;
	outerCutoffAngle = outerAngle;
}

void SpotLight::SetAttenuation(float constant, float linear, float quadratic)
{
	constantAttenuation = constant;
	linearAttenuation = linear;
	quadraticAttenuation = quadratic;
}
