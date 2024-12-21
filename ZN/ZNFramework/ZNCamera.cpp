#include "ZNCamera.h"
#include <math.h>

ZNFramework::ZNCamera::ZNCamera()
	: viewMatrix(ZNMatrix4())
	, projectionMatrix(ZNMatrix4())
{
}

void ZNFramework::ZNCamera::SetView(const ZNVector3& pos, const ZNVector3& target, const ZNVector3& up)
{
	ZNVector3 direction = target - pos;
	ZNVector3 axisZ = ZNVector3(direction).Normalize();
	ZNVector3 axisX = ZNVector3::Cross(up, axisZ).Normalize();
	ZNVector3 axisY = ZNVector3::Cross(axisZ, axisX).Normalize();

	float tX = -ZNVector3::Dot(axisX, pos);
	float tY = -ZNVector3::Dot(axisY, pos);
	float tZ = -ZNVector3::Dot(axisZ, pos);

	ZNMatrix4 mat(
		axisX.x, axisY.x, axisZ.x, 0.0f,
		axisX.y, axisY.y, axisZ.y, 0.0f,
		axisX.z, axisY.z, axisZ.z, 0.0f,
		tX, tY, tZ, 1.0f);

	SetView(mat);
}

void ZNFramework::ZNCamera::SetView(const ZNMatrix4& m)
{
	this->viewMatrix = m;
}

void ZNFramework::ZNCamera::SetProjection(const ZNMatrix4& m)
{
	this->projectionMatrix = m;
}

void ZNFramework::ZNCamera::SetPerspective(float fov, float aspect, float nearZ, float farZ)
{
	float f = 1.0f / tan(fov / 2.0f);
	float fRange = farZ / (farZ - nearZ);

	ZNMatrix4 mat(
		f / aspect, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, fRange, 1.0f,
		0.0f, 0.0f, -fRange * nearZ, 0.0f);

	SetProjection(mat);
}

void ZNFramework::ZNCamera::SetOrthographic(float width, float height, float nearZ, float farZ)
{
	ZNMatrix4 mat(
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, 2.0f / (nearZ - farZ), 0.0f,
		0.0f, 0.0f, (farZ + nearZ) / (nearZ - farZ), 1.0f);

	SetProjection(mat);
}
