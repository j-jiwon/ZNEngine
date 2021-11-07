#pragma once
#include <math.h>
#include "ZNMatrix4.h"
#include "ZNVector4.h"

using namespace ZNFramework;

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

ZNVector4 ZNVector4::operator*(float f) const
{
	return ZNVector4(x * f, y * f, z * f, w * f);
}

ZNVector4 ZNVector4::operator*(const ZNMatrix4& m) const
{
	return ZNVector4(x * m._11 + y * m._21 + z * m._31 + w * m._41
					, x * m._12 + y * m._22 + z * m._32 + w * m._42
					, x * m._13 + y * m._23 + z * m._33 + w * m._43
					, x * m._14 + y * m._24 + z * m._34 + w * m._44);
}

ZNVector4& ZNFramework::ZNVector4::operator+=(const ZNVector4& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
	return *this;
}

ZNVector4& ZNFramework::ZNVector4::operator+=(float f)
{
	x += f;
	y += f;
	z += f;
	w += f;
	return *this;
}

ZNVector4& ZNFramework::ZNVector4::operator-=(const ZNVector4& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
	return *this;
}

ZNVector4& ZNVector4::operator*=(const ZNVector4& v)
{
	ZNVector4 vec(*this);
	x = v.x * vec.x;
	y = v.y * vec.y;
	z = v.z * vec.z;
	w = v.w * vec.w;
	return *this;
}

ZNVector4& ZNVector4::operator*=(const ZNMatrix4& m)
{
	ZNVector4 v(*this);
	x = v.x * m._11 + v.y * m._21 + v.z * m._31 + v.w * m._41;
	y = v.x * m._12 + v.y * m._22 + v.z * m._32 + v.w * m._42;
	z = v.x * m._13 + v.y * m._23 + v.z * m._33 + v.w * m._43;
	w = v.x * m._14 + v.y * m._24 + v.z * m._34 + v.w * m._44;
	return *this;
}

ZNVector4& ZNFramework::ZNVector4::operator*=(float f)
{
	x = x * f;
	y = y * f;
	z = z * f;
	w = w * f;
	return *this;
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
	if (lengthSq > 0.0f) t = 1 / sqrtf(lengthSq);

	this->x = x * t;
	this->y = y * t;
	this->z = z * t;
	this->w = w * t;
	return *this;
}
