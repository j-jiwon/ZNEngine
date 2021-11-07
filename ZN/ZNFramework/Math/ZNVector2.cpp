#pragma once
#include <math.h>
#include "ZNMatrix2.h"
#include "ZNVector2.h"

using namespace ZNFramework;

ZNVector2::ZNVector2()
	:x(0.0f), y(0.0f)
{
}

ZNVector2::ZNVector2(const ZNVector2& v)
{
	x = v.x;
	y = v.y;
}

ZNVector2::ZNVector2(float _x, float _y)
	:x(_x), y(_y)
{
}

bool ZNVector2::operator==(const ZNVector2& v) const
{
	return x == v.x && y == v.y;
}

bool ZNVector2::operator!=(const ZNVector2& v) const
{
	return x != v.x || y != v.y;
}

float ZNVector2::Dot(const ZNVector2& v1, const ZNVector2& v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

float ZNVector2::Cross(const ZNVector2& v1, const ZNVector2& v2)
{
	return v1.x * v2.y - v1.y * v2.x;
}

ZNVector2 ZNVector2::operator+(const ZNVector2& v) const
{
	return ZNVector2(x + v.x, y + v.y);
}

ZNVector2 ZNVector2::operator-(const ZNVector2& v) const
{
	return ZNVector2(x - v.x, y - v.y);
}

ZNVector2 ZNVector2::operator*(float f) const
{
	return ZNVector2(x * f, y * f);
}

ZNVector2 ZNVector2::operator*(const ZNVector2& v) const
{
	return ZNVector2(x * v.x, y * v.y);
}

ZNVector2 ZNVector2::operator*(const ZNMatrix2& m) const
{

	return ZNVector2(x * m._11 + y * m._21,
					 x * m._12 + y * m._22);
}

ZNVector2& ZNFramework::ZNVector2::operator+=(const ZNVector2& v)
{
	x += v.x;
	y += v.y;
	return *this;
}

ZNVector2& ZNFramework::ZNVector2::operator+=(float f)
{
	x += f;
	y += f;
	return *this;
}

ZNVector2& ZNFramework::ZNVector2::operator-=(const ZNVector2& v)
{
	x -= v.x;
	y -= v.y;
	return *this;
}

ZNVector2& ZNVector2::operator*=(const ZNVector2& v)
{
	ZNVector2 vec(x, y);
	this->x = vec.x * v.x;
	this->y = vec.y * v.y;
	return *this;
}

ZNVector2& ZNVector2::operator*=(const ZNMatrix2& m)
{
	x = x * m._11 + y * m._21;
	y = y * m._12 + y * m._22;
	return *this;
}

ZNVector2& ZNFramework::ZNVector2::operator*=(float f)
{
	x = x * f;
	y = y * f;
	return *this;
}

float ZNVector2::Length() const
{
	return sqrt(x * x + y * y);
}

float ZNVector2::LengthSq() const
{
	return x * x + y * y;
}

ZNVector2& ZNVector2::Normalize()
{
	float lengthSq = x * x + y * y;
	float t = 0.0f;
	if (lengthSq > 0.0f) t = 1 / sqrtf(lengthSq);

	this->x = x * t;
	this->y = y * t;
	return *this;
}
