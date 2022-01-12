#include "ZNColor.h"

using namespace ZNFramework;

ZNFramework::ZNColor::ZNColor()
	:r(0.0f), g(0.0f), b(0.0f), a(1.0f)
{
}

ZNFramework::ZNColor::ZNColor(ARGB32 color)
	:r(static_cast<float>(color.r) / 255.0f)
	, g(static_cast<float>(color.g) / 255.0f)
	, b(static_cast<float>(color.b) / 255.0f)
	, a(static_cast<float>(color.a) / 255.0f)
{
}

ZNFramework::ZNColor::ZNColor(ARGB32 color)
	:r(static_cast<float>(color.r) / 255.0f)
	, g(static_cast<float>(color.g) / 255.0f)
	, b(static_cast<float>(color.b) / 255.0f)
	, a(static_cast<float>(color.a) / 255.0f)
{
}

ZNFramework::ZNColor::ZNColor(RGBA32 color)
	:r(static_cast<float>(color.r) / 255.0f)
	, g(static_cast<float>(color.g) / 255.0f)
	, b(static_cast<float>(color.b) / 255.0f)
	, a(static_cast<float>(color.a) / 255.0f)
{
}

ZNFramework::ZNColor::ZNColor(ZNColor& color)
	:r(color.r), g(color.g), b(color.b), a(color.a)
{
}

ZNFramework::ZNColor::ZNColor(float r, float g, float b, float a)
	:r(r), g(g), b(b), a(a)
{
}

ZNColor ZNFramework::ZNColor::operator+(const ZNColor& c) const
{
	return ZNColor(r + c.r, g + c.g, b + c.b, a + c.a);
}

ZNColor ZNFramework::ZNColor::operator-(const ZNColor& c) const
{
	return ZNColor(r - c.r, g - c.g, b - c.b, a - c.a);
}

ZNColor ZNFramework::ZNColor::operator*(const ZNColor& c) const
{
	return ZNColor(r * c.r, g * c.g, b * c.b, a * c.a);
}

ZNColor ZNFramework::ZNColor::operator*(const float f) const
{
	return ZNColor(r * f, g * f, b * f, a *f);
}

ZNColor& ZNFramework::ZNColor::operator+=(const ZNColor& c)
{
	r += c.r;
	g += c.g;
	b += c.b;
	a += c.a;
	return *this;
}

ZNColor& ZNFramework::ZNColor::operator-=(const ZNColor& c)
{
	r -= c.r;
	g -= c.g;
	b -= c.b;
	a -= c.a;
	return *this;
}

ZNColor& ZNFramework::ZNColor::operator*=(const ZNColor& c)
{
	r *= c.r;
	g *= c.g;
	b *= c.b;
	a *= c.a;
	return *this;
}

ZNColor& ZNFramework::ZNColor::operator*=(float f)
{
	r *= f;
	g *= f;
	b *= f;
	a *= f;
	return *this;
}
