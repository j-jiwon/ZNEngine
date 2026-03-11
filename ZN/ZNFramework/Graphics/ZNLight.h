#pragma once

namespace ZNFramework
{
	// Forward declarations
	class ZNVector3;
	struct LightData;

	enum class LightType
	{
		Directional,
		Point,
		Spot
	};

	class ZNLight
	{
	public:
		ZNLight() = default;
		virtual ~ZNLight() = default;

		virtual LightType GetType() const = 0;
		virtual LightData GetLightData() const = 0;

		// Common properties
		virtual void SetIntensity(float intensity) = 0;
		virtual void SetColor(const ZNVector3& color) = 0;
		virtual void SetAmbientIntensity(float ambient) = 0;

		virtual float GetIntensity() const = 0;
		virtual ZNVector3 GetColor() const = 0;
		virtual float GetAmbientIntensity() const = 0;
	};

	// Directional Light
	class ZNDirectionalLight : public ZNLight
	{
	public:
		ZNDirectionalLight() = default;
		virtual ~ZNDirectionalLight() = default;

		virtual void SetDirection(const ZNVector3& direction) = 0;
		virtual ZNVector3 GetDirection() const = 0;
	};

	// Spot Light
	class ZNSpotLight : public ZNLight
	{
	public:
		ZNSpotLight() = default;
		virtual ~ZNSpotLight() = default;

		virtual void SetPosition(const ZNVector3& position) = 0;
		virtual void SetDirection(const ZNVector3& direction) = 0;
		virtual void SetCutoffAngle(float innerAngle, float outerAngle) = 0;
		virtual void SetAttenuation(float constant, float linear, float quadratic) = 0;

		virtual ZNVector3 GetPosition() const = 0;
		virtual ZNVector3 GetDirection() const = 0;
		virtual float GetInnerCutoffAngle() const = 0;
		virtual float GetOuterCutoffAngle() const = 0;
	};
}
