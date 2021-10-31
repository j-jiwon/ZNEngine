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

		static float Dot(const ZNVector2& v1, const ZNVector2& v2);
		static float Cross(const ZNVector2& v1, const ZNVector2& v2);

		ZNVector2 operator + (const ZNVector2& v) const;
		ZNVector2 operator - (const ZNVector2& v) const;
		ZNVector2 operator * (const float f) const;
		ZNVector2 operator * (const ZNVector2& v) const;
		ZNVector2 operator * (const ZNMatrix2& m) const;

		ZNVector2& operator += (const ZNVector2& v);
		ZNVector2& operator += (float f);
		ZNVector2& operator -= (const ZNVector2& v);
		ZNVector2& operator *= (const ZNVector2& v);
		ZNVector2& operator *= (const ZNMatrix2& m);
		ZNVector2& operator *= (float f);

		float Length() const;
		float LengthSq() const;
		ZNVector2& Normalize();

		union
		{
			struct
			{
				float x, y;
			};
			float value[2];
		};
	};
}
