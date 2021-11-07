#include "ZNLinearTransform2.h"
#include <math.h>

using namespace ZNFramework;

ZNLinearTransform2::ZNLinearTransform2()
	: matrix2()
{
}

ZNLinearTransform2::ZNLinearTransform2(const ZNMatrix2& m)
	:matrix2(m)
{
}

bool ZNLinearTransform2::operator==(const ZNLinearTransform2& lt) const
{
	return matrix2 == lt.matrix2;
}

bool ZNLinearTransform2::operator!=(const ZNLinearTransform2& lt) const
{
	return matrix2 != lt.matrix2;
}

ZNLinearTransform2& ZNLinearTransform2::Scale(float x, float y)
{
	matrix2._11 *= x; matrix2._12 *= y;
	matrix2._21 *= x; matrix2._22 *= y;

	return *this;
}

ZNLinearTransform2& ZNLinearTransform2::Scale(const ZNVector2& v)
{
	return Scale(v.x, v.y);
}

ZNLinearTransform2& ZNLinearTransform2::Rotate(float angle)
{
	float cosA = cos(angle);
	float sinA = sin(angle);

	matrix2 *= ZNMatrix2 (cosA, sinA, -sinA, cosA);
	return *this;
}

ZNLinearTransform2& ZNLinearTransform2::Multiply(const ZNMatrix2& m)
{
	matrix2 *= m;
	return *this;
}
