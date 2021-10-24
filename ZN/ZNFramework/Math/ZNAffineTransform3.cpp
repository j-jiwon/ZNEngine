#pragma once
#include "ZNVector3.h"
#include "ZNMatrix3.h"
#include "ZNMatrix4.h"
#include "ZNLinearTransform3.h"
#include "ZNAffineTransform3.h"

using namespace ZNFramework;

ZNAffineTransform3::ZNAffineTransform3()
	:matrix3(), translation()
{
}

ZNAffineTransform3::ZNAffineTransform3(const ZNLinearTransform3& lt, const ZNVector3& v)
	: matrix3(lt.matrix3), translation(v)
{
}

ZNAffineTransform3::ZNAffineTransform3(const ZNMatrix4& m)
	: matrix3(m._11, m._12, m._13,
		m._21, m._22, m._23,
		m._31, m._32, m._33)
	, translation(m._41, m._42, m._43)
{
}

bool ZNAffineTransform3::operator==(const ZNAffineTransform3& at) const
{
	if (translation == at.translation)
		return matrix3 == at.matrix3;
	return false;
}

bool ZNAffineTransform3::operator!=(const ZNAffineTransform3& at) const
{
	if (translation != at.translation)
		return true;
	return matrix3 != at.matrix3;
}

ZNAffineTransform3& ZNAffineTransform3::Translate(float x, float y, float z)
{
	translation.x += x;
	translation.y += y;
	translation.z += z;
	return *this;
}

ZNAffineTransform3& ZNAffineTransform3::Translate(const ZNVector3& v)
{
	translation.x += v.x;
	translation.y += v.y;
	translation.z += v.z;
	return *this;
}

ZNAffineTransform3& ZNAffineTransform3::Multiply(const ZNAffineTransform3& at)
{
	matrix3 *= at.matrix3;
	translation = translation * at.matrix3 + at.translation;
	return *this;
}

ZNAffineTransform3& ZNAffineTransform3::Multiply(const ZNLinearTransform3& lt)
{
	matrix3 *= lt.matrix3;
	translation *= lt.matrix3;
	return *this;
}

ZNMatrix4 ZNAffineTransform3::Matrix4() const
{
	return ZNMatrix4(
		matrix3._11, matrix3._12, matrix3._13, 0.0f,
		matrix3._21, matrix3._22, matrix3._23, 0.0f,
		matrix3._31, matrix3._32, matrix3._32, 0.0f,
		translation.x, translation.y, translation.z, 1.0f
	);
}
