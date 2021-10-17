#pragma once
#include "ZNMatrix2.h"
#include "ZNVector2.h"

class ZNLinearTransform2
{
public:
	ZNLinearTransform2();
	ZNLinearTransform2(const ZNMatrix2& m);

	bool operator == (const ZNLinearTransform2& lt) const;
	bool operator != (const ZNLinearTransform2& lt) const;

	ZNLinearTransform2& Scale(float x, float y);
	ZNLinearTransform2& Scale(const ZNVector2& v);
	ZNLinearTransform2& Rotate(float angle);
	ZNLinearTransform2& Multiply(const ZNMatrix2& m);

	ZNMatrix2 matrix2;
};