#pragma once
#include "ZNVector2.h"

class ZNMatrix2 
{
public:
	ZNMatrix2();
	ZNMatrix2(float f11, float f12, float f21, float f22);
	ZNMatrix2(ZNVector2 v1, ZNVector2 v2);
	ZNMatrix2(const ZNMatrix2& m);

	bool operator == (const ZNMatrix2& m) const;
	bool operator != (const ZNMatrix2& m) const;

	ZNMatrix2 operator + (const ZNMatrix2& m) const;
	ZNMatrix2 operator - (const ZNMatrix2 & m) const;
	ZNMatrix2 operator * (const ZNMatrix2& m) const;
	ZNMatrix2 operator * (const float f) const;
	ZNMatrix2& operator += (const ZNMatrix2& m);
	ZNMatrix2& operator *= (const ZNMatrix2 & m);

	ZNMatrix2& Transpose();
	ZNMatrix2& Inverse();
	ZNMatrix2 Identity();

	float _11, _12, _21, _22;
};