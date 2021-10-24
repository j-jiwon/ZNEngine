#include "ZNMatrix2.h"
#include "ZNMatrix3.h"
#include "ZNVector2.h"
#include "ZNLinearTransform2.h"
#include "ZNAffineTransform2.h"

ZNAffineTransform2::ZNAffineTransform2(const ZNLinearTransform2& lt, const ZNVector2& v)
	: matrix2(lt.matrix2), translation(v)
{
}

ZNAffineTransform2::ZNAffineTransform2(const ZNMatrix3& m)
	: matrix2(m._11, m._12, m._21, m._22), translation(m._31, m._32)
{
}

ZNAffineTransform2::ZNAffineTransform2(float x, float y)
	: matrix2(ZNMatrix2()), translation(x, y)
{
}

bool ZNAffineTransform2::operator==(const ZNAffineTransform2& at) const
{
	if (translation == at.translation)
		return matrix2 == at.matrix2;
	return false;
}

bool ZNAffineTransform2::operator!=(const ZNAffineTransform2& at) const
{
	if (translation != at.translation)
		return true;
	return matrix2 != at.matrix2;
}

ZNAffineTransform2& ZNAffineTransform2::Translate(float x, float y)
{
	translation.x += x;
	translation.y += y;
	return *this;
}

ZNAffineTransform2& ZNAffineTransform2::Translate(const ZNVector2& v)
{
	translation.x += v.x;
	translation.y += v.y;
	return *this;
}

ZNAffineTransform2& ZNAffineTransform2::Multiply(const ZNAffineTransform2& at)
{
	matrix2 *= at.matrix2;
	translation = translation * at.matrix2 + at.translation;
	return *this;
}

ZNAffineTransform2& ZNAffineTransform2::Multiply(const ZNLinearTransform2& lt)
{
	matrix2 *= lt.matrix2;
	translation *= lt.matrix2;
	return *this;
}

ZNMatrix3 ZNAffineTransform2::Matrix3() const
{
	return ZNMatrix3(
		matrix2._11, matrix2._12, 0.0f,
		matrix2._21, matrix2._22, 0.0f,
		translation.x, translation.y, 1.0f
	);
}
