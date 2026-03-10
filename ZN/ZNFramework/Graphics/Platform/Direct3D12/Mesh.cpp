#include "Mesh.h"
#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "ConstantBuffer.h"
#include "TableDescriptorHeap.h"
#include "Texture.h"
#include "Material.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNMesh* CreateMesh()
	{
		return new Mesh();
	}
}

void Mesh::Init(const vector<Vertex>& vertrexBuffer, const vector<uint32>& indexBuffer)
{
	CreateVertexBuffer(vertrexBuffer);
	CreateIndexBuffer(indexBuffer);
}

void Mesh::Render()
{
	CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	queue->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	queue->CommandList()->IASetVertexBuffers(0, 1, &vertexBufferView); // Slot: (0~15)
	queue->CommandList()->IASetIndexBuffer(&indexBufferView);

	// Get camera from GraphicsContext
	ZNCamera* camera = GraphicsContext::GetInstance().GetCamera();

	// Build TransformMatrices (cbTransform : register(b0))
	TransformMatrices transformMatrices;
	transformMatrices.world = transform.GetWorldMatrix();
	transformMatrices.view = camera ? camera->ViewMatrix() : ZNMatrix4(); // Identity if no camera
	transformMatrices.projection = camera ? camera->ProjectionMatrix() : ZNMatrix4(); // Identity if no camera

	ConstantBuffer* constantBuffer = GraphicsContext::GetInstance().GetAs<ConstantBuffer>();
	TableDescriptorHeap* tableDescHeap = GraphicsContext::GetInstance().GetAs<TableDescriptorHeap>();

	// Set MVP matrices (b0)
	D3D12_CPU_DESCRIPTOR_HANDLE handle1 = constantBuffer->PushData(0, &transformMatrices, sizeof(TransformMatrices));
	tableDescHeap->SetCBV(handle1, CBV_REGISTER::b0);

	// Use Material if available, otherwise fallback to legacy texture path
	if (material)
	{
		material->Bind();
	}
	else if (texture)
	{
		tableDescHeap->SetSRV(texture->GetCpuHandle(), SRV_REGISTER::t0);
	}

	tableDescHeap->CommitTable();

	queue->CommandList()->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void Mesh::SetTexture(ZNTexture* inTexture)
{
	texture = dynamic_cast<Texture*>(inTexture);
}

void Mesh::SetMaterial(ZNMaterial* inMaterial)
{
	material = dynamic_cast<Material*>(inMaterial);
}

void Mesh::CreateVertexBuffer(const vector<Vertex>& buffer)
{
	vertexCount = static_cast<uint32>(buffer.size());
	uint32 bufferSize = vertexCount * sizeof(Vertex);

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
	::memcpy(vertexDataBuffer, &buffer[0], bufferSize);
	vertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = bufferSize;
}

void Mesh::CreateIndexBuffer(const vector<uint32>& buffer)
{
	indexCount = static_cast<uint32>(buffer.size());
	uint32 bufferSize = indexCount * sizeof(uint32);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	device->Device()->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer));

	void* indexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	indexBuffer->Map(0, &readRange, &indexDataBuffer);
	::memcpy(indexDataBuffer, &buffer[0], bufferSize);
	indexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT; // uint32
	indexBufferView.SizeInBytes = bufferSize;
}
