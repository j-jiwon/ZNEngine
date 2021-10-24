#pragma once
#include "ZNVector3.h"
#include "ZNMatrix3.h"
#include "ZNMatrix4.h"
#include "ZNLinearTransform3.h"

class ZNAffineTransform3
{
public:
	ZNAffineTransform3(const ZNLinearTransform3& lt, const ZNVector3& v);
	ZNAffineTransform3(const ZNMatrix4& m);
	ZNAffineTransform3(float x, float y, float z);

	bool operator == (const ZNAffineTransform3& at) const;
	bool operator != (const ZNAffineTransform3& at) const;

	ZNAffineTransform3& Translate(float x, float y, float z);
	ZNAffineTransform3& Translate(const ZNVector3& v);

	ZNAffineTransform3& Multiply(const ZNAffineTransform3& at);
	ZNAffineTransform3& Multiply(const ZNLinearTransform3& lt);

	ZNMatrix4 Matrix4() const;

	ZNMatrix3 matrix3;
	ZNVector3 translation;
};