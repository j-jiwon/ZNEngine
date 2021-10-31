#pragma once
#include "ZNMatrix4.h"
#include "ZNVector3.h"

namespace ZNFramework
{
	class ZNMatrix4;
	class ZNVector4
	{
	public:
		ZNVector4();
		ZNVector4(const ZNVector4& v);
		ZNVector4(float x, float y, float z, float w);

		bool operator == (const ZNVector4& v) const;
		bool operator != (const ZNVector4& v) const;

		static float Dot(const ZNVector4& v1, const ZNVector4& v2);

		ZNVector4 operator + (const ZNVector4& v) const;
		ZNVector4 operator - (const ZNVector4& v) const;
		ZNVector4 operator * (float f) const;
		ZNVector4 operator * (const ZNMatrix4& m) const;

		ZNVector4& operator += (const ZNVector4& v);
		ZNVector4& operator += (float f);
		ZNVector4& operator -= (const ZNVector4& v);
		ZNVector4& operator *= (const ZNVector4& v);
		ZNVector4& operator *= (const ZNMatrix4& m);
		ZNVector4& operator *= (float f);

		float Length() const;
		float LengthSq() const;
		ZNVector4& Normalize();

		union
		{
			struct
			{
				float x, y, z, w;
			};
			float value[4];
		};
	};
}
