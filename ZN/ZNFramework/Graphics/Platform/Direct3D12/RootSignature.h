#pragma once
#include "ZNUtils.h"
#include "../../ZNRootSignature.h"

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
