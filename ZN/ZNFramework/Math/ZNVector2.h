#pragma once
#include "ZNMatrix2.h"

namespace ZNFramework
{
	class ZNMatrix2;
	class ZNVector2
	{
	public:
		ZNVector2();
		ZNVector2(const ZNVector2& v);
		ZNVector2(float x, float y);

		bool operator == (const ZNVector2& v) const;
		bool operator != (const ZNVector2& v) const;

		float Dot(const ZNVector2& v1, const ZNVector2& v2);
		float Cross(const ZNVector2& v1, const ZNVector2& v2);

		ZNVector2 operator + (const ZNVector2& v) const;
		ZNVector2 operator - (const ZNVector2& v) const;
		ZNVector2 operator * (const float f) const;
		ZNVector2 operator * (const ZNVector2& v) const;
		ZNVector2 operator * (const ZNMatrix2& m) const;

		ZNVector2& operator *= (const ZNVector2& v);
		ZNVector2& operator *= (const ZNMatrix2& m);

		float Length() const;
		float LengthSq() const;
		ZNVector2& Normalize();

		float x, y;
	};
}
