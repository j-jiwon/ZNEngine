#pragma once
namespace ZNFramework
{
	class ZNMatrix2;
	class ZNColor
	{	
		// 32 bit  = 4 * 8 bit
		union ARGB32
		{
			struct{
				unsigned char a, r, g, b;  // 0 ~ 255
			};
			unsigned char value[4];
			unsigned int color;
		};

		union RGBA32
		{
			struct{
				unsigned char r, g, b, a; // 0 ~ 255
			};
			unsigned char value[4];
			unsigned int color;
		};

	public:
		ZNColor();
		ZNColor(ARGB32 color);   // 32 bit color
		ZNColor(RGBA32 color);   // 32 bit color
		ZNColor(ZNColor& color); // 128 bit color (linear color)
		ZNColor(float r, float g, float b, float a);

		ZNColor operator + (const ZNColor& c) const;
		ZNColor operator - (const ZNColor& c) const;
		ZNColor operator * (const ZNColor& c) const;
		ZNColor operator * (const float f) const;
		
		ZNColor& operator += (const ZNColor& c);
		ZNColor& operator -= (const ZNColor& c);
		ZNColor& operator *= (const ZNColor& c);
		ZNColor& operator *= (float f);


		// 128 bit = 4 * 32 bit
		union
		{
			struct
			{
				float r, g, b, a;
			};
			float value[4];
		};
		
	};
}
