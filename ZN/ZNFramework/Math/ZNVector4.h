#pragma once
#include "ZNMatrix4.h"
#

class ZNVector4
{
public:
	ZNVector4();
	ZNVector4(const ZNVector4& v);  // copy
	ZNVector4(float x, float y, float z, float w);

	bool operator == (const ZNVector4& v) const;
	bool operator != (const ZNVector4& v) const;

	float Dot(const ZNVector4& v1, const ZNVector4& v2);

	ZNVector4 operator + (const ZNVector4& v) const;
	ZNVector4 operator - (const ZNVector4& v) const;
	ZNVector4 operator * (const float f) const;
	// ZNVector4 operator * (const ZNMatrix4& m) const;

	float Length() const;
	float LengthSq() const;
	ZNVector4& Normalize();

	float x, y, z, w;
};