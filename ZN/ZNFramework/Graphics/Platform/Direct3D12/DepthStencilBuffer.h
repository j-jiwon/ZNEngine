#pragma once
#include "Graphics/ZNDepthStencilBuffer.h"
#include "ZNUtils.h"

using namespace std;

namespace ZNFramework
{
	class DepthStencilBuffer : public ZNDepthStencilBuffer
	{
	public:
		void Init() override;
		void InitInternal(DXGI_FORMAT inDsvFormat = DXGI_FORMAT_D32_FLOAT);

		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCpuHandle() { return dsvHandle; }
		DXGI_FORMAT GetDSVFormat() { return dsvFormat; }

	private:
		ComPtr<ID3D12Resource> dsvBuffer;
		ComPtr<ID3D12DescriptorHeap> dsvHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
		DXGI_FORMAT dsvFormat = {};
	};
}
