#include "ZNMatrix4.h"

ZNMatrix4::ZNMatrix4()
	: _11(0), _12(0), _13(0), _14(0)
	, _21(0), _22(0), _23(0), _24(0)
	, _31(0), _32(0), _33(0), _34(0)
	, _41(0), _42(0), _43(0), _44(0)
{
}

ZNMatrix4::ZNMatrix4(float f11, float f12, float f13, float f14
	, float f21, float f22, float f23, float f24
	, float f31, float f32, float f33, float f34
	, float f41, float f42, float f43, float f44)
	: _11(f11), _12(f12), _13(f13), _14(f14)
	, _21(f21), _22(f22), _23(f23), _24(f24)
	, _31(f31), _32(f32), _33(f33), _34(f34)
	, _41(f41), _42(f42), _43(f43), _44(f44) 
{
}

ZNMatrix4::ZNMatrix4(ZNVector4 v1, ZNVector4 v2, ZNVector4 v3, ZNVector4 v4)
	: _11(v1.x), _12(v1.y), _13(v1.z), _14(v1.w)
	, _21(v2.x), _22(v2.y), _23(v2.z), _24(v2.w)
	, _31(v3.x), _32(v3.y), _33(v3.z), _34(v3.w)
	, _41(v4.x), _42(v4.y), _43(v4.z), _44(v4.w)
{
}

ZNMatrix4::ZNMatrix4(const ZNMatrix4& m)
{
	_11 = m._11; _12 = m._12; _13 = m._13; _14 = m._14;
	_21 = m._21; _22 = m._22; _23 = m._23; _24 = m._24;
	_31 = m._31; _32 = m._32; _33 = m._33; _34 = m._34;
	_41 = m._41; _42 = m._42; _43 = m._43; _44 = m._44;
}

bool ZNMatrix4::operator==(const ZNMatrix4& m) const
{
	return _11 == m._11 && _12 == m._12 && _13 == m._13 && _14 == m._14
		&& _21 == m._21 && _22 == m._22 && _23 == m._23 && _24 == m._24
		&& _31 == m._31 && _32 == m._32 && _33 == m._33 && _34 == m._34
		&& _41 == m._41 && _42 == m._42 && _43 == m._43 && _44 == m._44;
}

bool ZNMatrix4::operator!=(const ZNMatrix4& m) const
{
	return _11 != m._11 || _12 != m._12 || _13 != m._13 || _14 != m._14
		|| _21 != m._21 || _22 != m._22 || _23 != m._23 || _24 != m._24
		|| _31 != m._31 || _32 != m._32 || _33 != m._33 || _34 != m._34
		|| _41 != m._41 || _42 != m._42 || _43 != m._43 || _44 != m._44;
}

ZNMatrix4 ZNMatrix4::operator+(const ZNMatrix4& m) const
{
	return ZNMatrix4(_11 + m._11, _12 + m._12, _13 + m._13, _14 + m._14,
					 _21 + m._21, _22 + m._22, _23 + m._23, _24 + m._24,
					 _31 + m._31, _32 + m._32, _33 + m._33, _34 + m._34,
					 _41 + m._41, _42 + m._42, _43 + m._43, _44 + m._44);
}

ZNMatrix4 ZNMatrix4::operator-(const ZNMatrix4& m) const
{
	return ZNMatrix4(_11 - m._11, _12 - m._12, _13 - m._13, _14 - m._14,
					 _21 - m._21, _22 - m._22, _23 - m._23, _24 - m._24,
					 _31 - m._31, _32 - m._32, _33 - m._33, _34 - m._34,
					 _41 - m._41, _42 - m._42, _43 - m._43, _44 - m._44);
}

ZNMatrix4 ZNMatrix4::operator*(const ZNMatrix4& m) const
{
	return ZNMatrix4( _11 * m._11 + _12 * m._21 + _13 * m._31 + _14 * m._41, _11 * m._12 + _12 * m._22 + _13 * m._32 + _14 * m._42, _11 * m._13 + _12 * m._23 + _13 * m._33 + _14 * m._43, _11 * m._14 + _12 * m._24 + _13 * m._34 + _14 * m._44
					, _21 * m._11 + _22 * m._21 + _23 * m._31 + _24 * m._41, _21 * m._12 + _22 * m._22 + _23 * m._32 + _24 * m._42, _21 * m._13 + _22 * m._23 + _23 * m._33 + _24 * m._43, _21 * m._14 + _22 * m._24 + _23 * m._34 + _24 * m._44
					, _31 * m._11 + _32 * m._21 + _33 * m._31 + _34 * m._41, _31 * m._12 + _32 * m._22 + _33 * m._32 + _34 * m._42, _31 * m._13 + _32 * m._23 + _33 * m._33 + _34 * m._43, _31 * m._14 + _32 * m._24 + _33 * m._34 + _34 * m._44
					, _41 * m._11 + _42 * m._21 + _43 * m._31 + _44 * m._41, _41 * m._12 + _42 * m._22 + _43 * m._32 + _44 * m._42, _41 * m._13 + _42 * m._23 + _43 * m._33 + _44 * m._43, _41 * m._14 + _42 * m._24 + _43 * m._34 + _44 * m._44);

}

ZNMatrix4 ZNMatrix4::operator*(const float f) const
{
	return ZNMatrix4(_11 * f, _12 * f, _13 * f, _14 * f,
					_21 * f, _22 * f, _23 * f, _24 * f,
					_31 * f, _32 * f, _33 * f, _34 * f,
					_41 * f, _42 * f, _43 * f, _44 * f);
}

ZNMatrix4& ZNMatrix4::Transpose()
{
	ZNMatrix4 m(*this);
	this->_12 = m._21; this->_13 = m._31; this->_41 = m._41;
	this->_21 = m._12; this->_23 = m._32; this->_24 = m._42;
	this->_31 = m._13; this->_32 = m._23; this->_34 = m._43;
	this->_41 = m._14; this->_42 = m._24; this->_43 = m._34;
	return *this;
}

ZNMatrix4& ZNMatrix4::Inverse()
{
	float det = Determinant();
	if (det != 0.0f)
	{
		float t = 1.0f / det;
		this->_11 = t * ((_23 * _34 * _42) - (_24 * _33 * _42) + (_24 * _32 * _43) - (_22 * _34 * _43) - (_23 * _32 * _44) + (_22 * _33 * _44));
		this->_12 = t * ((_14 * _33 * _42) - (_13 * _34 * _42) - (_14 * _32 * _43) + (_12 * _34 * _43) + (_13 * _32 * _44) - (_12 * _33 * _44));
		this->_13 = t * ((_13 * _24 * _42) - (_14 * _23 * _42) + (_14 * _22 * _43) - (_12 * _24 * _43) - (_13 * _22 * _44) + (_12 * _23 * _44));
		this->_14 = t * ((_14 * _23 * _32) - (_13 * _24 * _32) - (_14 * _22 * _33) + (_12 * _24 * _33) + (_13 * _22 * _34) - (_12 * _23 * _34));
		this->_21 = t * ((_24 * _33 * _41) - (_23 * _34 * _41) - (_24 * _31 * _43) + (_21 * _34 * _43) + (_23 * _31 * _44) - (_21 * _33 * _44));
		this->_22 = t * ((_13 * _34 * _41) - (_14 * _33 * _41) + (_14 * _31 * _43) - (_11 * _34 * _43) - (_13 * _31 * _44) + (_11 * _33 * _44));
		this->_23 = t * ((_14 * _23 * _41) - (_13 * _24 * _41) - (_14 * _21 * _43) + (_11 * _24 * _43) + (_13 * _21 * _44) - (_11 * _23 * _44));
		this->_24 = t * ((_13 * _24 * _31) - (_14 * _23 * _31) + (_14 * _21 * _33) - (_11 * _24 * _33) - (_13 * _21 * _34) + (_11 * _23 * _34));
		this->_31 = t * ((_22 * _34 * _41) - (_24 * _32 * _41) + (_24 * _31 * _42) - (_21 * _34 * _42) - (_22 * _31 * _44) + (_21 * _32 * _44));
		this->_32 = t * ((_14 * _32 * _41) - (_12 * _34 * _41) - (_14 * _31 * _42) + (_11 * _34 * _42) + (_12 * _31 * _44) - (_11 * _32 * _44));
		this->_33 = t * ((_12 * _24 * _41) - (_14 * _22 * _41) + (_14 * _21 * _42) - (_11 * _24 * _42) - (_12 * _21 * _44) + (_11 * _22 * _44));
		this->_34 = t * ((_14 * _22 * _31) - (_12 * _24 * _31) - (_14 * _21 * _32) + (_11 * _24 * _32) + (_12 * _21 * _34) - (_11 * _22 * _34));
		this->_41 = t * ((_23 * _32 * _41) - (_22 * _33 * _41) - (_23 * _31 * _42) + (_21 * _33 * _42) + (_22 * _31 * _43) - (_21 * _32 * _43));
		this->_42 = t * ((_12 * _33 * _41) - (_13 * _32 * _41) + (_13 * _31 * _42) - (_11 * _33 * _42) - (_12 * _31 * _43) + (_11 * _32 * _43));
		this->_43 = t * ((_13 * _22 * _41) - (_12 * _23 * _41) - (_13 * _21 * _42) + (_11 * _23 * _42) + (_12 * _21 * _43) - (_11 * _22 * _43));
		this->_44 = t * ((_12 * _23 * _31) - (_13 * _22 * _31) + (_13 * _21 * _32) - (_11 * _23 * _32) - (_12 * _21 * _33) + (_11 * _22 * _33));
	}
	return *this;
}

ZNMatrix4 ZNMatrix4::Identity()
{
	return ZNMatrix4(1.0f, 0.0f, 0.0f, 0.0f
					,0.0f, 1.0f, 0.0f, 0.0f
					,0.0f, 0.0f, 1.0f, 0.0f
					,0.0f, 0.0f, 0.0f, 1.0f);
}

float ZNMatrix4::Determinant() const
{
	return (_14 * _23 * _32 * _41) - (_13 * _24 * _32 * _41) - (_14 * _22 * _33 * _41) + (_12 * _24 * _33 * _41) + (_13 * _22 * _34 * _41) - (_12 * _23 * _34 * _41)
		 - (_14 * _23 * _31 * _42) + (_13 * _24 * _31 * _42) + (_14 * _21 * _33 * _42) - (_11 * _24 * _33 * _42) - (_13 * _21 * _34 * _42) + (_11 * _23 * _34 * _42)
		 + (_14 * _22 * _31 * _43) - (_12 * _24 * _31 * _43) - (_14 * _21 * _32 * _43) + (_11 * _24 * _32 * _43) + (_12 * _21 * _34 * _43) - (_11 * _22 * _34 * _43)
		 - (_13 * _22 * _31 * _44) + (_12 * _23 * _31 * _44) + (_13 * _21 * _32 * _44) - (_11 * _23 * _32 * _44) - (_12 * _21 * _33 * _44) + (_11 * _22 * _33 * _44);
}