#pragma once
#include "Graphics/ZNRootSignature.h"
#include "ZNUtils.h"

namespace ZNFramework
{
	class RootSignature : public ZNRootSignature
	{
	public:
		void Init() override;
	
		ComPtr<ID3D12RootSignature> GetSignature() { return signature; }

	private:
		void CreateRootSignature();
		void CreateSamplerDesc();

	private:
		ComPtr<ID3D12RootSignature> signature;
		D3D12_STATIC_SAMPLER_DESC samplerDesc;
	};
}
