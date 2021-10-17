#include "ZNLinearTransform3.h"
#include "ZNMatrix3.h"
#include <math.h>

ZNLinearTransform3::ZNLinearTransform3()
	:matrix3()
{
}

ZNLinearTransform3::ZNLinearTransform3(const ZNMatrix3& m)
	:matrix3(m)
{
}

bool ZNLinearTransform3::operator==(const ZNLinearTransform3& lt) const
{
	return matrix3 == lt.matrix3;
}

bool ZNLinearTransform3::operator!=(const ZNLinearTransform3& lt) const
{
	return matrix3 != lt.matrix3;
}

ZNLinearTransform3& ZNLinearTransform3::Scale(float x, float y, float z)
{
	matrix3._11 *= x; matrix3._12 *= y; matrix3._13 *= z;
	matrix3._21 *= x; matrix3._22 *= y; matrix3._23 *= z;
	matrix3._31 *= x; matrix3._32 *= y; matrix3._33 *= z;
	return *this;
}

ZNLinearTransform3& ZNLinearTransform3::Scale(const ZNVector3& v)
{
	return Scale(v.x, v.y, v.z);
}

ZNLinearTransform3& ZNLinearTransform3::RotateX(float angle)
{
	float cosA = cos(angle);
	float sinA = sin(angle);
	matrix3 *= ZNMatrix3(1.0f, 0.0f, 0.0f,
						0.0f, cosA, -sinA,
						0.0f, sinA, cosA);
	return *this;
}

ZNLinearTransform3& ZNLinearTransform3::RotateY(float angle)
{
	float cosA = cos(angle);
	float sinA = sin(angle);
	matrix3 *= ZNMatrix3(cosA, 0.0f, -sinA,
		                 0.0f, 1.0f, 0.0f,
		                 sinA, 0.0f, cosA);
	return *this;
}

ZNLinearTransform3& ZNLinearTransform3::RotateZ(float angle)
{
	float cosA = cos(angle);
	float sinA = sin(angle);
	matrix3 *= ZNMatrix3(cosA, sinA, 0.0f,
		                -sinA, cosA, 0.0f,
		                 0.0f, 0.0f, 1.0f);
	return *this;
}

ZNLinearTransform3& ZNLinearTransform3::Rotate(const ZNVector3& axis, float angle)
{
	if (angle == 0) return *this;
	float cosA = cos(angle);
	float sinA = sin(angle);
	ZNVector3 au = axis;
	au.Normalize();

	matrix3 *= ZNMatrix3(cosA + au.x * au.x * (1 - cosA), au.x * au.y * (1 - cosA) - au.z * sinA, au.x * au.z * (1 - cosA) + au.y * sinA,
						au.y * au.x * (1 - cosA) + au.z * sinA, cosA + au.y * au.y * (1 - cosA), au.y * au.z * (1 - cosA) - au.x * sinA,
						au.z * au.x * (1 - cosA) - au.y * sinA, au.z * au.y * (1 - cosA) + au.x * sinA, cosA + au.z * au.z * (1 - cosA));
	return *this;
}

ZNLinearTransform3& ZNLinearTransform3::Multiply(const ZNMatrix3& m)
{
	matrix3 *= m;
	return *this;
}
