#include "Mesh.h"
#include "GraphicsDevice.h"
#include "CommandQueue.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNMesh* CreateMesh()
	{
		return new Mesh();
	}
}

void Mesh::Init(const std::vector<Vertex>& vec)
{
	vertexCount = static_cast<UINT>(vec.size());
	UINT bufferSize = vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	device->Device()->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer));

	// Copy the triangle data to the vertex buffer.
	void* vertexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
	vertexBuffer->Map(0, &readRange, &vertexDataBuffer);
	::memcpy(vertexDataBuffer, &vec[0], bufferSize);
	vertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = bufferSize;
}

void Mesh::Render()
{
	CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	queue->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	queue->CommandList()->IASetVertexBuffers(0, 1, &vertexBufferView); // Slot: (0~15)
	queue->CommandList()->DrawInstanced(vertexCount, 1, 0, 0);
}
