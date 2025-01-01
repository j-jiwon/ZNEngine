#pragma once
#include "ZNUtils.h"
#include "Graphics/ZNTableDescriptorHeap.h"

namespace ZNFramework
{
	class TableDescriptorHeap : public ZNTableDescriptorHeap
	{
	public:
		void Init(uint32 inCount) override;

		void Clear();
		void SetCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg);
		void SetSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, SRV_REGISTER reg);
		void CommitTable();

		ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return descHeap; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(CBV_REGISTER reg);
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(SRV_REGISTER reg);

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint8 reg);

	private:
		ComPtr<ID3D12DescriptorHeap> descHeap;
		uint64 handleSize = 0;
		uint64 groupSize = 0;
		uint64 groupCount = 0;
		uint32 currentGroupIndex = 0;
	};
}