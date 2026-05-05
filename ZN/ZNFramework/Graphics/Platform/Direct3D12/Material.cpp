#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include "ConstantBuffer.h"
#include "TableDescriptorHeap.h"
#include "CommandQueue.h"
#include "ZNFramework.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNMaterial* CreateMaterial()
	{
		return new Material();
	}
}

void Material::Init()
{
	// Initialize with default parameters
	params = MaterialParams();

	// Clear texture array
	for (auto& tex : textures)
	{
		tex = nullptr;
	}
}

void Material::SetShader(ZNShader* inShader)
{
	shader = inShader;
}

void Material::SetTexture(TextureType type, ZNTexture* texture)
{
	size_t index = static_cast<size_t>(type);
	if (index < textures.size())
	{
		textures[index] = dynamic_cast<Texture*>(texture);
	}
}

void Material::SetParams(const MaterialParams& inParams)
{
	params = inParams;
}

void Material::Bind()
{
	CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	bool isMRTMode = (queue->GetGBufferManager() != nullptr);
	bool isForwardPass = queue->IsForwardPass();

	// Bind shader (skip in MRT mode unless in forward pass)
	if (shader && (!isMRTMode || isForwardPass))
	{
		shader->Bind();
	}

	// Bind material parameters to constant buffer (b1)
	ConstantBuffer* constantBuffer = GraphicsContext::GetInstance().GetAs<ConstantBuffer>();
	TableDescriptorHeap* tableDescHeap = GraphicsContext::GetInstance().GetAs<TableDescriptorHeap>();

	D3D12_CPU_DESCRIPTOR_HANDLE paramsHandle = constantBuffer->PushData(1, &params, sizeof(params));
	tableDescHeap->SetCBV(paramsHandle, CBV_REGISTER::b1);

	// Bind textures (t0 ~ t4)
	for (size_t i = 0; i < textures.size(); ++i)
	{
		if (textures[i])
		{
			SRV_REGISTER srvRegister = static_cast<SRV_REGISTER>(static_cast<int>(SRV_REGISTER::t0) + i);
			tableDescHeap->SetSRV(textures[i]->GetCpuHandle(), srvRegister);
		}
	}
}
