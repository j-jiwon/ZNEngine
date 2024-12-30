#include "ConstantBuffer.h"
#include "CommandQueue.h"
#include "GraphicsDevice.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNConstantBuffer* CreateConstantBuffer()
	{
		return new ConstantBuffer();
	}
}

ConstantBuffer::~ConstantBuffer()
{
	if (cbvBuffer)
	{
		if (cbvBuffer != nullptr)
		{
			cbvBuffer->Unmap(0, nullptr);
		}
		cbvBuffer = nullptr;
	}
}

void ConstantBuffer::Init(uint32 inSize, uint32 inCount)
{
	elementSize = (inSize + 255) & ~255;
	elementCount = inCount;

	CreateBuffer();
	CreateView();
}

void ConstantBuffer::Clear()
{
	currentIndex = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE ConstantBuffer::PushData(int32 inRootParamIndex, void* inBuffer, uint32 inSize)
{
	assert(currentIndex < elementSize);

	::memcpy(&mappedBuffer[currentIndex * elementSize], inBuffer, inSize);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCpuHandle(currentIndex);
	currentIndex++;

	return cpuHandle;
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGpuVirtualAddress(uint32 index)
{
	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = cbvBuffer->GetGPUVirtualAddress();
	objCBAddress += index * elementSize;
	return objCBAddress;
}

D3D12_CPU_DESCRIPTOR_HANDLE ZNFramework::ConstantBuffer::GetCpuHandle(uint32 index)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuHandleBegin, index * handleIncrementSize);
}

void ConstantBuffer::CreateBuffer()
{
	uint32 bufferSize = elementSize * elementCount;
	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	device->Device()->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&cbvBuffer));

	cbvBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedBuffer));
}

void ConstantBuffer::CreateView()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvDesc = {};
	cbvDesc.NumDescriptors = elementCount;
	cbvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	cbvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	device->Device()->CreateDescriptorHeap(&cbvDesc, IID_PPV_ARGS(&cbvHeap));
	cpuHandleBegin = cbvHeap->GetCPUDescriptorHandleForHeapStart();
	handleIncrementSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (uint32 i = 0; i < elementCount; ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = GetCpuHandle(i);
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = cbvBuffer->GetGPUVirtualAddress() + static_cast<uint64>(elementSize) * i;
		cbvDesc.SizeInBytes = elementSize;   // CB size is required to be 256-byte aligned.
		device->Device()->CreateConstantBufferView(&cbvDesc, cbvHandle);
	}
}
