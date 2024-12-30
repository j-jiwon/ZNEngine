#include "TableDescriptorHeap.h"
#include "Graphics/ZNGraphicsContext.h"
#include "GraphicsDevice.h"
#include "CommandQueue.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNTableDescriptorHeap* CreateTableDescriptorHeap()
	{
		return new TableDescriptorHeap();
	}
}

void TableDescriptorHeap::Init(uint32 inCount)
{
	groupCount = inCount;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = inCount * REGISTER_COUNT;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	device->Device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descHeap));
	handleSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	groupSize = handleSize * REGISTER_COUNT;
}

void TableDescriptorHeap::Clear()
{
	currentGroupIndex = 0;
}

void TableDescriptorHeap::SetCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);

	uint32 destRange = 1;
	uint32 srcRange = 1;
	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	device->Device()->CopyDescriptors(1, &destHandle, &destRange, 1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void TableDescriptorHeap::CommitTable()
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = descHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += currentGroupIndex * groupSize;
	CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	queue->CommandList()->SetGraphicsRootDescriptorTable(0, handle);

	currentGroupIndex++;
}

D3D12_CPU_DESCRIPTOR_HANDLE TableDescriptorHeap::GetCPUHandle(CBV_REGISTER reg)
{
	return GetCPUHandle(static_cast<uint32>(reg));
}

D3D12_CPU_DESCRIPTOR_HANDLE TableDescriptorHeap::GetCPUHandle(uint32 reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = descHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += currentGroupIndex * groupSize;
	handle.ptr += reg * handleSize;
	return handle;
}
