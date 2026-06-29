#pragma once
#include "../../ZNLight.h"
#include "../../../Math/ZNVector3.h"

namespace ZNFramework::Platform::Direct3D
{
	class PointLight : public ZNPointLight
	{
	public:
		PointLight();
		virtual ~PointLight() = default;

		virtual LightType GetType() const override { return LightType::Point; }
		virtual LightData GetLightData() const override;

		virtual void SetIntensity(float intensity) override        { this->intensity = intensity; }
		virtual void SetColor(const ZNVector3& color) override    { this->color = color; }
		virtual void SetAmbientIntensity(float ambient) override  { this->ambientIntensity = ambient; }

		virtual float     GetIntensity()        const override { return intensity; }
		virtual ZNVector3 GetColor()            const override { return color; }
		virtual float     GetAmbientIntensity() const override { return ambientIntensity; }

		virtual void SetPosition(const ZNVector3& position) override { this->position = position; }
		virtual void SetRadius(float radius) override                { this->radius = radius; }
		virtual void SetAttenuation(float constant, float linear, float quadratic) override;

		virtual ZNVector3 GetPosition()              const override { return position; }
		virtual float     GetRadius()                const override { return radius; }
		virtual float     GetConstantAttenuation()   const override { return constantAttenuation; }
		virtual float     GetLinearAttenuation()     const override { return linearAttenuation; }
		virtual float     GetQuadraticAttenuation()  const override { return quadraticAttenuation; }

	private:
		ZNVector3 position          = ZNVector3(0.f, 0.f, 0.f);
		float     intensity         = 1.f;
		ZNVector3 color             = ZNVector3(1.f, 1.f, 1.f);
		float     ambientIntensity  = 0.f;
		float     radius            = 5.f;
		float     constantAttenuation  = 1.f;
		float     linearAttenuation    = 0.09f;
		float     quadraticAttenuation = 0.032f;
	};
}
