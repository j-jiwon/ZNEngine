#pragma once
#include "ZNUtils.h"
#include "ZNFramework.h"

namespace ZNFramework
{
	class ZNConstantBuffer;
	class ConstantBuffer : public ZNConstantBuffer
	{
	public:
		~ConstantBuffer();
		void Init(uint32 inSize, uint32 inCount) override;

		void Clear();
		D3D12_CPU_DESCRIPTOR_HANDLE PushData(int32 inRootParamIndex, void* inBuffer, uint32 inSize);
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint32 index);
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32 index);


	private:
		void CreateBuffer();
		void CreateView();

	private:
		ComPtr<ID3D12Resource> cbvBuffer;
		BYTE* mappedBuffer = nullptr;
		uint32 elementSize = 0;
		uint32 elementCount = 0;
		uint32 currentIndex = 0;

		ComPtr<ID3D12DescriptorHeap> cbvHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleBegin = {};
		uint32 handleIncrementSize = 0;
	};
}
