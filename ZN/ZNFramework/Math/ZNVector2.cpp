#include "ZNVector2.h"
#include <math.h>

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

ZNVector2 ZNVector2::operator*(const float f) const
{
	return ZNVector2(x * f, y * f);
}

/*
ZNVector2 ZNVector2::operator*(const ZNMatrix2& m) const
{
	return ZNVector2(v.x * m._11 + v.y * m._21
					, v.x * m._12 + v.y * m._22);
}
*/

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
	if (lengthSq > 0.0f) t = 1 / lengthSq;

	this->x = x * t;
	this->y = y * t;
	return *this;
}
