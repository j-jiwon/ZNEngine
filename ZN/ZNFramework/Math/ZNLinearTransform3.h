#pragma once
#include "ZNMatrix3.h"
#include "ZNVector3.h"

namespace ZNFramework
{
	class ZNLinearTransform3
	{
	public:
		ZNLinearTransform3();
		ZNLinearTransform3(const ZNMatrix3& m);

		bool operator == (const ZNLinearTransform3& lt) const;
		bool operator != (const ZNLinearTransform3& lt) const;

		ZNLinearTransform3& Scale(float x, float y, float z);
		ZNLinearTransform3& Scale(const ZNVector3& v);
		ZNLinearTransform3& RotateX(float angle);
		ZNLinearTransform3& RotateY(float angle);
		ZNLinearTransform3& RotateZ(float angle);
		ZNLinearTransform3& Rotate(const ZNVector3& axis, float angle);
		ZNLinearTransform3& Multiply(const ZNMatrix3& m);

		ZNMatrix3 matrix3;
	};
}
