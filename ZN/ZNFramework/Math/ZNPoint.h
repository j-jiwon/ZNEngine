#pragma once
#include "ZNVector2.h"

namespace ZNFramework
{
	class ZNPoint
	{
	public:
		ZNPoint(void)
			: x(0), y(0)
		{
		}
		ZNPoint(const ZNVector2& v)
			: x(v.x), y(v.y)
		{
		}
		ZNPoint(float fx, float fy)
			: x(fx), y(fy)
		{
		}
		bool operator != (const ZNPoint& pt) const
		{
			return x != pt.x || y != pt.y;
		}
		bool operator == (const ZNPoint& pt) const
		{
			return x == pt.x && y == pt.y;
		}
		ZNPoint operator - (const ZNPoint& pt) const
		{
			return ZNPoint(x - pt.x, y - pt.y);
		}
		ZNPoint operator - (float p) const
		{
			return ZNPoint(x - p, y - p);
		}
		ZNPoint operator + (const ZNPoint& pt) const
		{
			return ZNPoint(x + pt.x, y + pt.y);
		}
		ZNPoint operator + (float p) const
		{
			return ZNPoint(x + p, y + p);
		}
		ZNPoint operator * (const ZNPoint& pt) const
		{
			return ZNPoint(x * pt.x, y * pt.y);
		}
		ZNPoint operator * (float p) const
		{
			return ZNPoint(x * p, y * p);
		}
		ZNPoint operator / (const ZNPoint& pt) const
		{
			return ZNPoint(x / pt.x, y / pt.y);
		}
		ZNPoint operator / (float p) const
		{
			return ZNPoint(x / p, y / p);
		}
		ZNPoint& operator -= (const ZNPoint& pt)
		{
			x -= pt.x;
			y -= pt.y;
			return *this;
		}
		ZNPoint& operator -= (float p)
		{
			x -= p;
			y -= p;
			return *this;
		}
		ZNPoint& operator += (const ZNPoint& pt)
		{
			x += pt.x;
			y += pt.y;
			return *this;
		}
		ZNPoint& operator += (float p)
		{
			x += p;
			y += p;
			return *this;
		}
		ZNPoint& operator *= (const ZNPoint& pt)
		{
			x *= pt.x;
			y *= pt.y;
			return *this;
		}
		ZNPoint& operator *= (float p)
		{
			x *= p;
			y *= p;
			return *this;
		}
		ZNPoint& operator /= (const ZNPoint& pt)
		{
			x /= pt.x;
			y /= pt.y;
			return *this;
		}
		ZNPoint& operator /= (float p)
		{
			x /= p;
			y /= p;
			return *this;
		}
		ZNVector2 Vector(void) const
		{
			return ZNVector2(x, y);
		}
		float x;
		float y;
	};
}