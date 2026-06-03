#pragma once
#include "../../ZNLight.h"
#include "../../../Math/ZNVector3.h"
#include "../../../Math/ZNMatrix4.h"

namespace ZNFramework::Platform::Direct3D
{
	class DirectionalLight : public ZNDirectionalLight
	{
	public:
		DirectionalLight();
		virtual ~DirectionalLight() = default;

		// ZNLight interface
		virtual LightType GetType() const override { return LightType::Directional; }
		virtual LightData GetLightData() const override;

		virtual void SetIntensity(float intensity) override { this->intensity = intensity; }
		virtual void SetColor(const ZNVector3& color) override { this->color = color; }
		virtual void SetAmbientIntensity(float ambient) override { this->ambientIntensity = ambient; }

		virtual float GetIntensity() const override { return intensity; }
		virtual ZNVector3 GetColor() const override { return color; }
		virtual float GetAmbientIntensity() const override { return ambientIntensity; }

		// ZNDirectionalLight interface
		virtual void SetDirection(const ZNVector3& direction) override { this->direction = direction; }
		virtual ZNVector3 GetDirection() const override { return direction; }

		// Shadow mapping
		void SetShadowBounds(float orthoSize, float nearPlane, float farPlane);
		void SetShadowFocusPoint(const ZNVector3& focusPoint) { shadowFocusPoint = focusPoint; }
		ZNMatrix4 GetLightViewMatrix() const;
		ZNMatrix4 GetLightProjectionMatrix() const;
		ZNMatrix4 GetLightViewProjectionMatrix() const;

	private:
		ZNVector3 direction = ZNVector3(0.0f, -1.0f, 0.0f);
		float intensity = 1.0f;
		ZNVector3 color = ZNVector3(1.0f, 1.0f, 1.0f);
		float ambientIntensity = 0.2f;

		// Shadow parameters
		float shadowOrthoSize = 20.0f;
		float shadowNear = 0.1f;
		float shadowFar = 100.0f;
		ZNVector3 shadowFocusPoint = ZNVector3(0.0f, 0.0f, 0.0f);
	};
}
