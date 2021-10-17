#pragma once
#include "ZNMatrix3.h"

class ZNVector3
{
public:
	ZNVector3();
	ZNVector3(const ZNVector3& v);  // copy
	ZNVector3(float x, float y, float z);

	bool operator == (const ZNVector3& v) const;
	bool operator != (const ZNVector3& v) const;

	float Dot(const ZNVector3& v1, const ZNVector3& v2);

	ZNVector3 operator + (const ZNVector3& v) const;
	ZNVector3 operator - (const ZNVector3& v) const;
	ZNVector3 operator * (const float f) const;
	// ZNVector3 operator * (const ZNMatrix3& m) const;

	float Length() const;
	float LengthSq() const;
	ZNVector3& Normalize();

	float x, y, z;
};