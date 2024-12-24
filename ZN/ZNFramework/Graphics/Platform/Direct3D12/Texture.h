#pragma once
#include "../../ZNTexture.h"
#include "ZNUtils.h"

namespace ZNFramework
{
	class Texture : public ZNTexture
	{
	public:
		Texture() {};
		~Texture() {};

		void SetTextureName() override;

		ComPtr<ID3D12Resource> resource;
	};
}