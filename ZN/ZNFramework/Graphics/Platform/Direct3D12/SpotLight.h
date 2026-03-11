#pragma once
#include "../../ZNLight.h"
#include "../../../Math/ZNVector3.h"

namespace ZNFramework::Platform::Direct3D
{
	class SpotLight : public ZNSpotLight
	{
	public:
		SpotLight();
		virtual ~SpotLight() = default;

		// ZNLight interface
		virtual LightType GetType() const override { return LightType::Spot; }
		virtual LightData GetLightData() const override;

		virtual void SetIntensity(float intensity) override { this->intensity = intensity; }
		virtual void SetColor(const ZNVector3& color) override { this->color = color; }
		virtual void SetAmbientIntensity(float ambient) override { this->ambientIntensity = ambient; }

		virtual float GetIntensity() const override { return intensity; }
		virtual ZNVector3 GetColor() const override { return color; }
		virtual float GetAmbientIntensity() const override { return ambientIntensity; }

		// ZNSpotLight interface
		virtual void SetPosition(const ZNVector3& position) override { this->position = position; }
		virtual void SetDirection(const ZNVector3& direction) override { this->direction = direction; }
		virtual void SetCutoffAngle(float innerAngle, float outerAngle) override;
		virtual void SetAttenuation(float constant, float linear, float quadratic) override;

		virtual ZNVector3 GetPosition() const override { return position; }
		virtual ZNVector3 GetDirection() const override { return direction; }
		virtual float GetInnerCutoffAngle() const override { return innerCutoffAngle; }
		virtual float GetOuterCutoffAngle() const override { return outerCutoffAngle; }

	private:
		ZNVector3 position = ZNVector3(0.0f, 0.0f, 0.0f);
		ZNVector3 direction = ZNVector3(0.0f, -1.0f, 0.0f);
		float intensity = 1.0f;
		ZNVector3 color = ZNVector3(1.0f, 1.0f, 1.0f);
		float ambientIntensity = 0.2f;

		// Spot light specific
		float innerCutoffAngle = 12.5f; // degrees
		float outerCutoffAngle = 17.5f; // degrees
		float constantAttenuation = 1.0f;
		float linearAttenuation = 0.09f;
		float quadraticAttenuation = 0.032f;
	};
}
