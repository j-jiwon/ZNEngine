#include "DirectionalLight.h"
#include "ZNFramework.h"
#include <cmath>

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

void DirectionalLight::SetShadowBounds(float orthoSize, float nearPlane, float farPlane)
{
	shadowOrthoSize = orthoSize;
	shadowNear = nearPlane;
	shadowFar = farPlane;
}

ZNMatrix4 DirectionalLight::GetLightViewMatrix() const
{
	// Normalize light direction
	ZNVector3 lightDir = direction;
	float len = std::sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
	if (len > 0.0001f)
	{
		lightDir.x /= len;
		lightDir.y /= len;
		lightDir.z /= len;
	}

	// Position light camera back along the negative light direction
	ZNVector3 lightPos;
	lightPos.x = shadowFocusPoint.x - lightDir.x * shadowFar * 0.5f;
	lightPos.y = shadowFocusPoint.y - lightDir.y * shadowFar * 0.5f;
	lightPos.z = shadowFocusPoint.z - lightDir.z * shadowFar * 0.5f;

	// Determine up vector (avoid parallel with light direction)
	ZNVector3 up;
	if (std::abs(lightDir.y) < 0.99f)
	{
		up = ZNVector3(0.0f, 1.0f, 0.0f);
	}
	else
	{
		up = ZNVector3(1.0f, 0.0f, 0.0f);
	}

	// Build LookAt matrix (Left-Handed)
	// zAxis = normalize(target - eye)
	ZNVector3 zAxis = lightDir;  // Already pointing from light to target

	// xAxis = normalize(cross(up, zAxis))
	ZNVector3 xAxis;
	xAxis.x = up.y * zAxis.z - up.z * zAxis.y;
	xAxis.y = up.z * zAxis.x - up.x * zAxis.z;
	xAxis.z = up.x * zAxis.y - up.y * zAxis.x;
	len = std::sqrt(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z);
	if (len > 0.0001f)
	{
		xAxis.x /= len;
		xAxis.y /= len;
		xAxis.z /= len;
	}

	// yAxis = cross(zAxis, xAxis)
	ZNVector3 yAxis;
	yAxis.x = zAxis.y * xAxis.z - zAxis.z * xAxis.y;
	yAxis.y = zAxis.z * xAxis.x - zAxis.x * xAxis.z;
	yAxis.z = zAxis.x * xAxis.y - zAxis.y * xAxis.x;

	// Translation
	float tx = -(xAxis.x * lightPos.x + xAxis.y * lightPos.y + xAxis.z * lightPos.z);
	float ty = -(yAxis.x * lightPos.x + yAxis.y * lightPos.y + yAxis.z * lightPos.z);
	float tz = -(zAxis.x * lightPos.x + zAxis.y * lightPos.y + zAxis.z * lightPos.z);

	// Row-major view matrix for mul(v, M) in HLSL
	// For row-vector multiplication: result[j] = sum_i(v[i] * M[i][j])
	// So rotation is transposed and translation goes in row 3
	return ZNMatrix4(
		xAxis.x, yAxis.x, zAxis.x, 0.0f,
		xAxis.y, yAxis.y, zAxis.y, 0.0f,
		xAxis.z, yAxis.z, zAxis.z, 0.0f,
		tx, ty, tz, 1.0f
	);
}

ZNMatrix4 DirectionalLight::GetLightProjectionMatrix() const
{
	// Orthographic projection for directional light (Left-Handed, depth [0,1])
	// For mul(v, M) with row_major matrix in HLSL:
	// result.x = v dot row0, result.y = v dot row1, etc.
	float w = shadowOrthoSize;
	float h = shadowOrthoSize;
	float n = shadowNear;
	float f = shadowFar;

	// Row-major orthographic projection matrix - matching camera's SetOrthographic pattern
	// Depth range [0,1] for DirectX
	return ZNMatrix4(
		2.0f / w, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f / h, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f / (f - n), 0.0f,
		0.0f, 0.0f, -n / (f - n), 1.0f
	);
}

ZNMatrix4 DirectionalLight::GetLightViewProjectionMatrix() const
{
	return GetLightViewMatrix() * GetLightProjectionMatrix();
}
