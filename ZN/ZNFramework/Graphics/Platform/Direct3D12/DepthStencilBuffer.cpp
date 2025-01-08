#include "DepthStencilBuffer.h"
#include "ZNFramework.h"
#include "Graphics/ZNGraphicsContext.h"
#include "GraphicsDevice.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNDepthStencilBuffer* CreateDepthStencilBuffer()
	{
		return new DepthStencilBuffer();
	}
}

void DepthStencilBuffer::Init()
{
	InitInternal();
}

void DepthStencilBuffer::InitInternal(DXGI_FORMAT inDsvFormat)
{
	ZNWindow* window = WindowContext::GetInstance().GetWindow();

	dsvFormat = inDsvFormat;
	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(dsvFormat, window->Width(), window->Height());
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	D3D12_CLEAR_VALUE optimizedClearValue = CD3DX12_CLEAR_VALUE(dsvFormat, 1.0f, 0);

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	device->Device()->CreateCommittedResource(&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&dsvBuffer));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	ThrowIfFailed(device->Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsvHeap)));

	dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	device->Device()->CreateDepthStencilView(dsvBuffer.Get(), nullptr, dsvHandle);
}
