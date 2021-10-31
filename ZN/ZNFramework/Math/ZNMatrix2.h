#pragma once
#include "ZNVector2.h"

namespace ZNFramework
{
	class ZNVector2;
	class ZNMatrix2
	{
	public:
		ZNMatrix2();
		ZNMatrix2(float f11, float f12, float f21, float f22);
		ZNMatrix2(const ZNVector2& v1, const ZNVector2& v2);

		bool operator == (const ZNMatrix2& m) const;
		bool operator != (const ZNMatrix2& m) const;

		ZNMatrix2 operator + (const ZNMatrix2& m) const;
		ZNMatrix2 operator - (const ZNMatrix2& m) const;
		ZNMatrix2 operator * (const ZNMatrix2& m) const;
		ZNMatrix2 operator * (float f) const;
		
		ZNMatrix2& operator += (const ZNMatrix2& m);
		ZNMatrix2& operator -= (const ZNMatrix2& m);
		ZNMatrix2& operator *= (const ZNMatrix2& m);
		ZNMatrix2& operator *= (float f);

		ZNMatrix2& Transpose();
		ZNMatrix2& Inverse();
		ZNMatrix2 Identity();

		float Determinant() const;

		union
		{
			struct
			{
				float _11, _12;
				float _21, _22;
			};
			struct
			{
				float m[2][2];
			};
			float value[4];
		};
	};
}
