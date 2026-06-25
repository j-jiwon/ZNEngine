#include "Mesh.h"
#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "ConstantBuffer.h"
#include "TableDescriptorHeap.h"
#include "Texture.h"
#include "Material.h"
#include "Shader.h"
#include "ShadowMap.h"
#include "DirectionalLight.h"

using namespace ZNFramework;

// Shadow pass constant buffer (matches shadow_depth.hlsli)
struct ShadowTransformCB
{
    ZNMatrix4 world;
    ZNMatrix4 lightViewProj;
};

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

	// Set forward light data (b2): dir light + spotlights + shadow for PBR forward shaders.
	// Reduced to max 2 spots to fit the lightViewProj matrix within 256-byte CB element.
	{
		struct FwdSpot {
			float pos[3];    float intensity;
			float dir[3];    float innerCutoff;
			float col[3];    float outerCutoff;
			float attConst;  float attLinear;  float attQuad;  float padding;
		}; // 64 bytes
		struct FwdLightCB {
			float dirDir[3];   float dirIntensity;   // 16
			float dirColor[3]; float dirAmbient;     // 16
			float viewPos[3];  int   numSpots;       // 16
			FwdSpot spots[2];                        // 128 (2 x 64)
			float lightVP[16];                       // 64 (row-major 4x4)
			float shadowBias;  float smW; float smH; float smPad; // 16
			// Total: 256 bytes
		} fwdLight = {};
		static_assert(sizeof(FwdLightCB) == 256, "FwdLightCB must be exactly 256 bytes");

		ZNDirectionalLight* dirLight = GraphicsContext::GetInstance().GetDirectionalLight();
		if (dirLight)
		{
			ZNVector3 d = dirLight->GetDirection();
			fwdLight.dirDir[0] = d.x; fwdLight.dirDir[1] = d.y; fwdLight.dirDir[2] = d.z;
			fwdLight.dirIntensity = dirLight->GetIntensity();
			ZNVector3 c = dirLight->GetColor();
			fwdLight.dirColor[0] = c.x; fwdLight.dirColor[1] = c.y; fwdLight.dirColor[2] = c.z;
			fwdLight.dirAmbient = dirLight->GetAmbientIntensity();

			auto* d3dDirLight = dynamic_cast<Platform::Direct3D::DirectionalLight*>(dirLight);
			if (d3dDirLight)
			{
				ZNMatrix4 lvp = d3dDirLight->GetLightViewProjectionMatrix();
				memcpy(fwdLight.lightVP, lvp.value, sizeof(float) * 16);
			}
		}
		if (camera)
		{
			ZNVector3 p = camera->GetPosition();
			fwdLight.viewPos[0] = p.x; fwdLight.viewPos[1] = p.y; fwdLight.viewPos[2] = p.z;
		}
		const auto& spotLights = GraphicsContext::GetInstance().GetSpotLights();
		int ns = 0;
		for (size_t si = 0; si < spotLights.size() && ns < 2; ++si)
		{
			if (!spotLights[si] || spotLights[si]->GetType() != LightType::Spot) continue;
			ZNSpotLight* sp = static_cast<ZNSpotLight*>(spotLights[si]);
			FwdSpot& s = fwdLight.spots[ns++];
			ZNVector3 p = sp->GetPosition(); s.pos[0]=p.x; s.pos[1]=p.y; s.pos[2]=p.z;
			s.intensity = sp->GetIntensity();
			ZNVector3 dd = sp->GetDirection(); s.dir[0]=dd.x; s.dir[1]=dd.y; s.dir[2]=dd.z;
			s.innerCutoff = cosf(sp->GetInnerCutoffAngle() * 3.14159f / 180.0f);
			ZNVector3 col = sp->GetColor(); s.col[0]=col.x; s.col[1]=col.y; s.col[2]=col.z;
			s.outerCutoff = cosf(sp->GetOuterCutoffAngle() * 3.14159f / 180.0f);
			s.attConst = sp->GetConstantAttenuation();
			s.attLinear = sp->GetLinearAttenuation();
			s.attQuad = sp->GetQuadraticAttenuation();
		}
		fwdLight.numSpots = ns;

		ShadowMap* sm = queue->GetShadowMap();
		if (sm)
		{
			fwdLight.shadowBias = 0.005f;
			fwdLight.smW = static_cast<float>(sm->GetWidth());
			fwdLight.smH = static_cast<float>(sm->GetHeight());
			tableDescHeap->SetSRV(sm->GetSRV(), SRV_REGISTER::t3);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE lh = constantBuffer->PushData(0, &fwdLight, sizeof(FwdLightCB));
		tableDescHeap->SetCBV(lh, CBV_REGISTER::b2);
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

void Mesh::RenderShadow(const ZNMatrix4& lightViewProj, ZNShader* shadowShader)
{
	CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	queue->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	queue->CommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
	queue->CommandList()->IASetIndexBuffer(&indexBufferView);

	// Bind shadow shader
	if (shadowShader)
	{
		shadowShader->Bind();
	}

	// Build ShadowTransformCB (cbShadowTransform : register(b0))
	ShadowTransformCB shadowTransform;
	shadowTransform.world = transform.GetWorldMatrix();
	shadowTransform.lightViewProj = lightViewProj;

	ConstantBuffer* constantBuffer = GraphicsContext::GetInstance().GetAs<ConstantBuffer>();
	TableDescriptorHeap* tableDescHeap = GraphicsContext::GetInstance().GetAs<TableDescriptorHeap>();

	// Set shadow transform matrices (b0)
	D3D12_CPU_DESCRIPTOR_HANDLE handle = constantBuffer->PushData(0, &shadowTransform, sizeof(ShadowTransformCB));
	tableDescHeap->SetCBV(handle, CBV_REGISTER::b0);

	tableDescHeap->CommitTable();

	queue->CommandList()->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
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
