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
		ComPtr<ID3D12RootSignature> signature;
	};
}
