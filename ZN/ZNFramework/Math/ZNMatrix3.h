#pragma once
#include "ZNVector3.h"

class ZNMatrix3
{
public:
	ZNMatrix3();
	ZNMatrix3(float f11, float f12, float f13,
		float f21, float f22, float f23,
		float f31, float f32, float f33);
	ZNMatrix3(ZNVector3 v1, ZNVector3 v2, ZNVector3 v3);
	ZNMatrix3(const ZNMatrix3& m);

	bool operator == (const ZNMatrix3& m) const;
	bool operator != (const ZNMatrix3& m) const;

	ZNMatrix3 operator + (const ZNMatrix3& m) const;
	ZNMatrix3 operator - (const ZNMatrix3& m) const;
	ZNMatrix3 operator * (const ZNMatrix3& m) const;
	ZNMatrix3 operator * (const float f) const;
	ZNMatrix3& operator += (const ZNMatrix3& m);
	ZNMatrix3& operator *= (const ZNMatrix3& m);


	ZNMatrix3& Transpose();
	ZNMatrix3& Inverse();
	ZNMatrix3 Identity();

	float Determinant() const;

	float _11, _12, _13;
	float _21, _22, _23;
	float _31, _32, _33;
};