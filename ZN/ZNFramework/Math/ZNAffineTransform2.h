#pragma once

namespace ZNFramework
{
	class ZNMatrix3;
	class ZNLinearTransform2;
	class ZNAffineTransform2
	{
	public:
		ZNAffineTransform2();
		ZNAffineTransform2(const ZNLinearTransform2& lt, const ZNVector2& v);
		ZNAffineTransform2(const ZNMatrix3& m);

		bool operator == (const ZNAffineTransform2& at) const;
		bool operator != (const ZNAffineTransform2& at) const;

		ZNAffineTransform2& Translate(float x, float y);
		ZNAffineTransform2& Translate(const ZNVector2& v);

		ZNAffineTransform2& Multiply(const ZNAffineTransform2& at);
		ZNAffineTransform2& Multiply(const ZNLinearTransform2& lt);

		ZNMatrix3 Matrix3() const;

		ZNMatrix2 matrix2;
		ZNVector2 translation;
	};
}
