#include "PointLight.h"
#include "ZNFramework.h"

using namespace ZNFramework;
using namespace ZNFramework::Platform::Direct3D;

namespace ZNFramework::Platform::Direct3D
{
	ZNPointLight* CreatePointLight()
	{
		return new PointLight();
	}
}

PointLight::PointLight()
{
	position          = ZNVector3(0.f, 0.f, 0.f);
	intensity         = 1.f;
	color             = ZNVector3(1.f, 1.f, 1.f);
	ambientIntensity  = 0.f;
	radius            = 5.f;
	constantAttenuation  = 1.f;
	linearAttenuation    = 0.09f;
	quadraticAttenuation = 0.032f;
}

LightData PointLight::GetLightData() const
{
	LightData data = {};
	data.lightType  = 1; // Point
	data.position   = position;
	data.intensity  = intensity;
	data.color      = color;
	data.ambientIntensity = ambientIntensity;
	data.constant   = constantAttenuation;
	data.linear     = linearAttenuation;
	data.quadratic  = quadraticAttenuation;
	return data;
}

void PointLight::SetAttenuation(float constant, float linear, float quadratic)
{
	constantAttenuation  = constant;
	linearAttenuation    = linear;
	quadraticAttenuation = quadratic;
}
