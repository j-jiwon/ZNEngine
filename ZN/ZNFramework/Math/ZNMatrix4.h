#pragma once
#include "ZNVector4.h"

class ZNMatrix4
{
public:
	ZNMatrix4();
	ZNMatrix4(float f11, float f12, float f13, float f14,
		float f21, float f22, float f23, float f24,
		float f31, float f32, float f33, float f34,
		float f41, float f42, float f43, float f44);
	ZNMatrix4(ZNVector4 v1, ZNVector4 v2, ZNVector4 v3, ZNVector4 v4);
	ZNMatrix4(const ZNMatrix4& m);

	bool operator == (const ZNMatrix4& m) const;
	bool operator != (const ZNMatrix4& m) const;

	ZNMatrix4 operator + (const ZNMatrix4& m) const;
	ZNMatrix4 operator - (const ZNMatrix4& m) const;
	ZNMatrix4 operator * (const ZNMatrix4& m) const;
	ZNMatrix4 operator * (const float f) const;

	ZNMatrix4& Transpose();
	ZNMatrix4& Inverse();
	ZNMatrix4 Identity();

	float Determinant() const;

	float _11, _12, _13, _14;
	float _21, _22, _23, _24;
	float _31, _32, _33, _34;
	float _41, _42, _43, _44;
};