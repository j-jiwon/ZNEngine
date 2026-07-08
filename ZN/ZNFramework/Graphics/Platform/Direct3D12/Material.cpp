#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include "ConstantBuffer.h"
#include "TableDescriptorHeap.h"
#include "CommandQueue.h"
#include "ZNFramework.h"
#include "../../ZNLight.h"
#include <algorithm>
#include <cstring>

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNMaterial* CreateMaterial()
	{
		return new Material();
	}
}

Material::~Material()
{
	for (auto& tex : textures)
	{
		delete tex;
		tex = nullptr;
	}
	// albedoSRVOverride is externally owned — not deleted here
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

	MaterialParams paramsToUse = params;
	if (queue->GetViewMode() == ViewMode::Wireframe)
	{
		paramsToUse.albedoColor = queue->IsCurrentObjectSelected()
			? ZNVector4(1.0f, 0.85f, 0.0f, 1.0f)   // yellow — selected object
			: ZNVector4(0.55f, 0.55f, 0.55f, 1.0f); // gray — all others
		paramsToUse.metallic  = 0.0f;
		paramsToUse.roughness = 1.0f;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE paramsHandle = constantBuffer->PushData(1, &paramsToUse, sizeof(paramsToUse));
	tableDescHeap->SetCBV(paramsHandle, CBV_REGISTER::b1);

	// Bind point lights at b3 during forward pass using a dedicated upload buffer.
	// forward_lit.hlsli uses register(b3) for cbPointLights; forward_pbr.hlsli and
	// screen_unlit.hlsli don't access b3, so setting it here is harmless to those shaders.
	if (isForwardPass)
	{
		struct FwdPointLightEntry {
			float position[3];
			float intensity;
			float color[3];
			float radius;
			float attenuationConstant;
			float attenuationLinear;
			float attenuationQuadratic;
			float padding;
		}; // 48 bytes

		static_assert(sizeof(FwdPointLightEntry) == 48, "FwdPointLightEntry size mismatch");

		struct FwdPointLightCB {
			int numPointLights;
			int pad[3];
			FwdPointLightEntry lights[8]; // 8 × 48 + 16 = 400 bytes → fits in 512-byte dedicated buffer
		};

		FwdPointLightCB plCB = {};
		const auto& pls = GraphicsContext::GetInstance().GetPointLights();
		int n = (int)(std::min)((int)pls.size(), 8);
		plCB.numPointLights = n;
		for (int i = 0; i < n; ++i)
		{
			ZNPointLight* pl = pls[i];
			ZNVector3 pos   = pl->GetPosition();
			ZNVector3 col   = pl->GetColor();
			auto& e = plCB.lights[i];
			e.position[0] = pos.x; e.position[1] = pos.y; e.position[2] = pos.z;
			e.intensity   = pl->GetIntensity();
			e.color[0]    = col.x; e.color[1] = col.y; e.color[2] = col.z;
			e.radius      = pl->GetRadius();
			e.attenuationConstant  = pl->GetConstantAttenuation();
			e.attenuationLinear    = pl->GetLinearAttenuation();
			e.attenuationQuadratic = pl->GetQuadraticAttenuation();
		}

		D3D12_CPU_DESCRIPTOR_HANDLE plHandle =
			queue->UpdateFwdPointLightBuffer(&plCB, sizeof(plCB));
		tableDescHeap->SetCBV(plHandle, CBV_REGISTER::b3);
	}

	// Bind textures (t0 ~ t4)
	for (size_t i = 0; i < textures.size(); ++i)
	{
		SRV_REGISTER srvRegister = static_cast<SRV_REGISTER>(static_cast<int>(SRV_REGISTER::t0) + i);
		if (i == 0 && hasAlbedoSRVOverride)
			tableDescHeap->SetSRV(albedoSRVOverride, SRV_REGISTER::t0);
		else if (textures[i])
			tableDescHeap->SetSRV(textures[i]->GetCpuHandle(), srvRegister);
	}
}
