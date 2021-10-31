#pragma once
#include "ZNMatrix3.h"

namespace ZNFramework
{
	class ZNMatrix3;
	class ZNVector3
	{
	public:
		ZNVector3();
		ZNVector3(float x, float y, float z);

		bool operator == (const ZNVector3& v) const;
		bool operator != (const ZNVector3& v) const;

		static float Dot(const ZNVector3& v1, const ZNVector3& v2);
		static ZNVector3 Cross(const ZNVector3& v1, const ZNVector3& v2);

		ZNVector3 operator + (const ZNVector3& v) const;
		ZNVector3 operator - (const ZNVector3& v) const;
		ZNVector3 operator * (float f) const;
		ZNVector3 operator * (const ZNMatrix3& m) const;

		ZNVector3& operator += (const ZNVector3& v);
		ZNVector3& operator += (float f);
		ZNVector3& operator -= (const ZNVector3& v);
		ZNVector3& operator *= (const ZNVector3& v);
		ZNVector3& operator *= (const ZNMatrix3& m);
		ZNVector3& operator *= (float f);

		float Length() const;
		float LengthSq() const;
		ZNVector3& Normalize();

		union
		{
			struct
			{
				float x, y, z;
			};
			float value[3];
		};
	};
}
