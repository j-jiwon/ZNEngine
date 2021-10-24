#pragma once
#include <math.h>
#include "ZNMatrix3.h"
#include "ZNVector3.h"

ZNVector3::ZNVector3()
	:x(0.0f), y(0.0f), z(0.0f)
{
}

ZNVector3::ZNVector3(const ZNVector3& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
}

ZNVector3::ZNVector3(float _x, float _y, float _z)
	:x(_x), y(_y), z(_z)
{
}

bool ZNVector3::operator==(const ZNVector3& v) const
{
	return x == v.x && y == v.y && z == v.z;
}

bool ZNVector3::operator!=(const ZNVector3& v) const
{
	return x != v.x || y != v.y || z != v.z;
}

float ZNVector3::Dot(const ZNVector3& v1, const ZNVector3& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

ZNVector3 ZNVector3::operator+(const ZNVector3& v) const
{
	return ZNVector3(x + v.x, y + v.y, z + v.z);
}

ZNVector3 ZNVector3::operator-(const ZNVector3& v) const
{
	return ZNVector3(x - v.x, y - v.y, z - v.z);
}

ZNVector3 ZNVector3::operator*(const float f) const
{
	return ZNVector3(x * f, y * f, z * f);
}

ZNVector3 ZNVector3::operator*(const ZNMatrix3& m) const
{
	return ZNVector3(x * m._11 + y * m._21 + z * m._31
					, x * m._12 + y * m._22 + z * m._32
					, x * m._13 + y * m._23 + z * m._33);
}

ZNVector3 ZNVector3::operator*=(const ZNVector3& v)
{
	x *= v.x;
	y *= v.y;
	z *= v.z;
	return *this;
}

ZNVector3 ZNVector3::operator*=(const ZNMatrix3& m)
{
	x = x * m._11 + y * m._21 + z * m._31;
	y = x * m._12 + y * m._22 + z * m._32;
	z = x * m._13 + y * m._23 + z * m._33;
	return *this;
}

float ZNVector3::Length() const
{
	return sqrt(x * x + y * y + z * z);
}

float ZNVector3::LengthSq() const
{
	return x * x + y * y + z * z;
}

ZNVector3& ZNVector3::Normalize()
{
	float lengthSq = x * x + y * y + z * z;
	float t = 0.0f;
	if (lengthSq > 0.0f) t = 1 / lengthSq;

	this->x = x * t;
	this->y = y * t;
	this->z = z * t;
	return *this;
}
