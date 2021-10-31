#pragma once
#include <math.h>
#include "ZNMatrix3.h"
#include "ZNVector3.h"

using namespace ZNFramework;

ZNVector3::ZNVector3()
	:x(0), y(0), z(0)
{
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

ZNVector3 ZNFramework::ZNVector3::Cross(const ZNVector3& v1, const ZNVector3& v2)
{
	return ZNVector3(v1.y * v2.z - v1.z * v2.y
					, v1.z * v2.x - v1.x * v2.z
					, v1.x * v2.y - v1.y * v2.x );
}

ZNVector3 ZNVector3::operator+(const ZNVector3& v) const
{
	return ZNVector3(x + v.x, y + v.y, z + v.z);
}

ZNVector3 ZNVector3::operator-(const ZNVector3& v) const
{
	return ZNVector3(x - v.x, y - v.y, z - v.z);
}

ZNVector3 ZNVector3::operator*(float f) const
{
	return ZNVector3(x * f, y * f, z * f);
}

ZNVector3 ZNVector3::operator*(const ZNMatrix3& m) const
{
	ZNVector3 vec(*this);
	vec.x = vec.x * m._11 + vec.y * m._21 + vec.z * m._31;
	vec.y = vec.x * m._12 + vec.y * m._22 + vec.z * m._32;
	vec.z = vec.x * m._13 + vec.y * m._23 + vec.z * m._33;
	return vec;
}

ZNVector3& ZNFramework::ZNVector3::operator+=(const ZNVector3& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}

ZNVector3& ZNFramework::ZNVector3::operator+=(float f)
{
	x += f;
	y += f;
	z += f;
	return *this;
}

ZNVector3& ZNFramework::ZNVector3::operator-=(const ZNVector3& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}

ZNVector3& ZNVector3::operator*=(const ZNVector3& v)
{
	ZNVector3 vec(*this);
	x = v.x * vec.x;
	y = v.y * vec.y;
	z = v.z * vec.z;
	return *this;
}

ZNVector3& ZNVector3::operator*=(const ZNMatrix3& m)
{
	ZNVector3 v(*this);
	x = v.x * m._11 + v.y * m._21 + v.z * m._31;
	y = v.x * m._12 + v.y * m._22 + v.z * m._32;
	z = v.x * m._13 + v.y * m._23 + v.z * m._33;
	return *this;
}

ZNVector3& ZNFramework::ZNVector3::operator*=(float f)
{
	x = x * f;
	y = y * f;
	z = z * f;
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
