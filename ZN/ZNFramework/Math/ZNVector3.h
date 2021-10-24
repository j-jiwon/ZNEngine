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

		float Dot(const ZNVector3& v1, const ZNVector3& v2);

		ZNVector3 operator + (const ZNVector3& v) const;
		ZNVector3 operator - (const ZNVector3& v) const;
		ZNVector3 operator * (float f) const;
		ZNVector3 operator * (const ZNMatrix3& m) const;

		ZNVector3& operator *= (const ZNVector3& v);
		ZNVector3& operator *= (const ZNMatrix3& m);

		float Length() const;
		float LengthSq() const;
		ZNVector3& Normalize();

		float x, y, z;
	};
}
