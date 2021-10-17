#include "ZNMatrix2.h"

ZNMatrix2::ZNMatrix2()
	:_11(0), _12(0), _21(0), _22(0)
{
}

ZNMatrix2::ZNMatrix2(float f11, float f12, float f21, float f22)
	:_11(f11), _12(f12), _21(f21), _22(f22)
{
}

ZNMatrix2::ZNMatrix2(ZNVector2 v1, ZNVector2 v2)
	: _11(v1.x), _12(v1.y), _21(v2.x), _22(v2.y)
{
}

ZNMatrix2::ZNMatrix2(const ZNMatrix2& m)
{
	_11 = m._11; _12 = m._12;
	_21 = m._21; _22 = m._22;
}

bool ZNMatrix2::operator==(const ZNMatrix2& m) const
{
	return _11 == m._11 && _12 == m._12 && _21 == m._21 && _22 == m._22;
}

bool ZNMatrix2::operator!=(const ZNMatrix2& m) const
{
	return _11 != m._11 || _12 != m._12 || _21 != m._21 || _22 != m._22;
}

ZNMatrix2 ZNMatrix2::operator+(const ZNMatrix2& m) const
{
	return ZNMatrix2(_11 + m._11, _12 + m._12,
					 _21 + m._21, _22 + m._22);
}

ZNMatrix2 ZNMatrix2::operator-(const ZNMatrix2& m) const
{
	return ZNMatrix2(_11 - m._11, _12 - m._12,
					 _21 - m._21, _22 - m._22);
}

ZNMatrix2 ZNMatrix2::operator*(const ZNMatrix2& m) const
{
	return ZNMatrix2(_11 * m._11 + _12 * m._21, _11 * m._12 + _12 * m._22,
					 _21 * m._11 + _22 * m._21, _21 * m._12 + _22 * m._22);
}

ZNMatrix2 ZNMatrix2::operator*(const float f) const
{
	return ZNMatrix2(_11 * f, _12 * f,
					 _21 * f, _22 * f);
}

ZNMatrix2& ZNMatrix2::operator+=(const ZNMatrix2& m)
{
	_11 += m._11; _12 += m._12;
	_21 += m._21; _22 += m._22;
	return *this;
}

ZNMatrix2& ZNMatrix2::operator*=(const ZNMatrix2& m)
{
	ZNMatrix2 mat(*this);
	_11 = m._11 * mat._11 + m._12 * mat._21;
	_12 = m._11 * mat._12 + m._12 * mat._22;
	_21 = m._21 * mat._11 + m._22 * mat._21;
	_22 = m._21 * mat._12 + m._22 * mat._22;
	return *this;
}

ZNMatrix2& ZNMatrix2::Transpose()
{
	float t = this->_12;
	this->_12 = _21;
	this->_21 = t;
	return *this;
}

ZNMatrix2& ZNMatrix2::Inverse()
{
	float det = _11 * _22 - _12 * _21;
	if (det != 0.0f)
	{
		float t = 1.0f / det;
		this->_11 = t * _22;  this->_12 = -t * _12;
		this->_21 = -t * _21; this->_22 = t * _11;
	}
	return *this;
}

ZNMatrix2 ZNMatrix2::Identity() 
{
	return ZNMatrix2(1.0f, 0.0f, 0.0f, 1.0f);
}