#pragma once
#include <math.h>
#include "ZNMatrix4.h"
#include "ZNVector4.h"

ZNVector4::ZNVector4()
	:x(0.0f), y(0.0f), z(0.0f), w(0.0f)
{
}

ZNVector4::ZNVector4(const ZNVector4& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
	w = v.w;
}

ZNVector4::ZNVector4(float _x, float _y, float _z, float _w)
	:x(_x), y(_y), z(_z), w(_w)
{
}

bool ZNVector4::operator==(const ZNVector4& v) const
{
	return x == v.x && y == v.y && z == v.z && w == v.w;
}

bool ZNVector4::operator!=(const ZNVector4& v) const
{
	return x != v.x || y != v.y || z != v.z || w != v.w;
}

float ZNVector4::Dot(const ZNVector4& v1, const ZNVector4& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

ZNVector4 ZNVector4::operator+(const ZNVector4& v) const
{
	return ZNVector4(x + v.x, y + v.y, z + v.z, w + v.w);
}

ZNVector4 ZNVector4::operator-(const ZNVector4& v) const
{
	return ZNVector4(x - v.x, y - v.y, z - v.z, w - v.w);
}

ZNVector4 ZNVector4::operator*(const float f) const
{
	return ZNVector4(x * f, y * f, z * f, w * f);
}

float ZNVector4::Length() const
{
	return sqrt(x * x + y * y + z * z + w * w);
}

float ZNVector4::LengthSq() const
{
	return x * x + y * y + z * z + w * w;
}

ZNVector4& ZNVector4::Normalize()
{
	float lengthSq = x * x + y * y + z * z + w * w;
	float t = 0.0f;
	if (lengthSq > 0.0f) t = 1 / lengthSq;

	this->x = x * t;
	this->y = y * t;
	this->z = z * t;
	this->w = w * t;
	return *this;
}
