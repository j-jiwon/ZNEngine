#include "ZNMatrix3.h"

using namespace ZNFramework;

ZNMatrix3::ZNMatrix3()
	:_11(1), _12(0), _13(0)
	,_21(0), _22(1), _23(0)
	,_31(0), _32(0), _33(1)
{
}

ZNMatrix3::ZNMatrix3(float f11, float f12, float f13,
	float f21, float f22, float f23,
	float f31, float f32, float f33)
	:_11(f11), _12(f12), _13(f13)
	,_21(f21), _22(f22), _23(f23)
	,_31(f31), _32(f32), _33(f33) 
{
}

ZNMatrix3::ZNMatrix3(const ZNVector3& v1, const ZNVector3& v2, const ZNVector3& v3)
	:_11(v1.x), _12(v1.y), _13(v1.z)
	,_21(v2.x), _22(v2.y), _23(v2.z)
	,_31(v3.x), _32(v3.y), _33(v3.z)
{
}

bool ZNMatrix3::operator==(const ZNMatrix3& m) const
{
	return _11 == m._11 && _12 == m._12 && _13 == m._13
		&& _21 == m._21 && _22 == m._22 && _23 == m._23
		&& _31 == m._31 && _32 == m._32 && _33 == m._33;
}

bool ZNMatrix3::operator!=(const ZNMatrix3& m) const
{
	return _11 != m._11 || _12 != m._12 || _13 != m._13
		|| _21 != m._21 || _22 != m._22 || _23 != m._23
		|| _31 != m._31 || _32 != m._32 || _33 != m._33;
}

ZNMatrix3 ZNMatrix3::operator+(const ZNMatrix3& m) const
{
	return ZNMatrix3(_11 + m._11, _12 + m._12, _13 + m._13
					,_21 + m._21, _22 + m._22, _23 + m._23
					,_31 + m._31, _32 + m._32, _33 + m._33);
}

ZNMatrix3 ZNMatrix3::operator-(const ZNMatrix3& m) const
{
	return ZNMatrix3(_11 - m._11, _12 - m._12, _13 - m._13
					,_21 - m._21, _22 - m._22, _23 - m._23
					,_31 - m._31, _32 - m._32, _33 - m._33);
}

ZNMatrix3 ZNMatrix3::operator*(const ZNMatrix3& m) const
{
	return ZNMatrix3(_11 * m._11 + _12 * m._21 + _13 * m._31
					, _11 * m._12 + _12 * m._22 + _13 * m._32
					, _11 * m._13 + _12 * m._23 + _13 * m._33
				    , _21 * m._11 + _22 * m._21 + _23 * m._31
					, _21 * m._12 + _22 * m._22 + _23 * m._32
					, _21 * m._13 + _22 * m._23 + _23 * m._33
				    , _31 * m._11 + _32 * m._21 + _33 * m._31
					, _31 * m._12 + _32 * m._22 + _33 * m._32
					, _31 * m._13 + _32 * m._23 + _33 * m._33);
}

ZNMatrix3 ZNMatrix3::operator*(float f) const
{
	return ZNMatrix3(_11 * f, _12 * f, _13 * f
					,_21 * f, _22 * f, _23 * f
					,_31 * f, _32 * f, _33 * f);
}

ZNMatrix3& ZNMatrix3::operator+=(const ZNMatrix3& m)
{
	_11 += m._11; _12 += m._12; _13 += m._13;
	_21 += m._21; _22 += m._22; _23 += m._23;
	_31 += m._31; _32 += m._32; _33 += m._33;
	return *this;
}

ZNMatrix3& ZNFramework::ZNMatrix3::operator-=(const ZNMatrix3& m)
{
	_11 -= m._11; _12 -= m._12; _13 -= m._13;
	_21 -= m._21; _22 -= m._22; _23 -= m._23;
	_31 -= m._31; _32 -= m._32; _33 -= m._33;
	return *this;
}

ZNMatrix3& ZNMatrix3::operator*=(const ZNMatrix3& m)
{
	ZNMatrix3 mat(*this);
	_11 = (mat._11 * m._11) + (mat._12 * m._21) + (mat._13 * m._31);
	_12 = (mat._11 * m._12) + (mat._12 * m._22) + (mat._13 * m._32);
	_13 = (mat._11 * m._13) + (mat._12 * m._23) + (mat._13 * m._33);
	_21 = (mat._21 * m._11) + (mat._22 * m._21) + (mat._23 * m._31);
	_22 = (mat._21 * m._12) + (mat._22 * m._22) + (mat._23 * m._32);
	_23 = (mat._21 * m._13) + (mat._22 * m._23) + (mat._23 * m._33);
	_31 = (mat._31 * m._11) + (mat._32 * m._21) + (mat._33 * m._31);
	_32 = (mat._31 * m._12) + (mat._32 * m._22) + (mat._33 * m._32);
	_33 = (mat._31 * m._13) + (mat._32 * m._23) + (mat._33 * m._33);
	return *this;
}

ZNMatrix3& ZNFramework::ZNMatrix3::operator*=(float f)
{
	_11 *= f; _12 *= f; _13 *= f;
	_21 *= f; _22 *= f; _23 *= f;
	_31 *= f; _32 *= f; _33 *= f;
	return *this;
}

ZNMatrix3 ZNMatrix3::Transpose() const
{
	return ZNMatrix3(_11, _21, _31
					, _12, _22, _32
					, _13, _23, _33);
}

ZNMatrix3 ZNMatrix3::Inverse() const
{
	float det = Determinant();
	ZNMatrix3 mat;
	if (det != 0.0f)
	{
		float t = 1.0f / det;
		mat._11 = (_22 * _33 - _23 * _32) * t;
		mat._13 = (_12 * _23 - _13 * _22) * t;
		mat._12 = (_13 * _32 - _12 * _33) * t;
		mat._21 = (_23 * _31 - _21 * _33) * t;
		mat._22 = (_11 * _33 - _13 * _31) * t;
		mat._23 = (_13 * _21 - _11 * _23) * t;
		mat._31 = (_21 * _32 - _22 * _31) * t;
		mat._32 = (_12 * _31 - _11 * _32) * t;
		mat._33 = (_11 * _22 - _12 * _21) * t;
	}
	return mat;
}

ZNMatrix3 ZNMatrix3::Identity()
{
	return ZNMatrix3(1.0f, 0.0f, 0.0f
					,0.0f, 1.0f, 0.0f
					,0.0f, 0.0f, 1.0f);
}

float ZNMatrix3::Determinant() const 
{
	return (_11 * _22 * _33) - (_11 * _23 * _32) - (_12 * _21 * _33)
		 + (_12 * _23 * _31) + (_13 * _21 * _32) - (_13 * _22 * _31);
}
