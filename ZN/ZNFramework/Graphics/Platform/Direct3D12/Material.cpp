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

	// Shared 1x1 black placeholder, bound to any texture slot (t0~t2) that has no real
	// texture set. Without this, an unset slot in the per-object descriptor table keeps
	// whatever a *different* object previously wrote there (the table is a rotating pool,
	// not cleared between draws), so gbuffer.hlsli's Normal/ARM "no texture" heuristic
	// (dot(sample,sample) < 0.01) can see stale, unrelated texture data instead of black.
	static Texture* GetDefaultBlackTexture()
	{
		static Texture* tex = nullptr;
		if (!tex)
		{
			tex = new Texture();
			tex->InitSolidColor(0, 0, 0, 0);
		}
		return tex;
	}

	static Texture* GetDefaultWhiteTexture()
	{
		static Texture* tex = nullptr;
		if (!tex)
		{
			tex = new Texture();
			tex->InitSolidColor(255, 255, 255, 255);
		}
		return tex;
	}
}

Material::~Material()
{
	for (size_t i = 0; i < textures.size(); ++i)
	{
		if (ownsTexture[i])
		{
			delete textures[i];
			textures[i] = nullptr;
		}
	}
	// albedoSRVOverride is externally owned — not deleted here
}

void Material::Init()
{
	params = MaterialParams();
	textures.fill(nullptr);
	ownsTexture.fill(false);

	// Force creation now (during scene setup, before the render loop starts) rather than
	// lazily on first Bind() — GPU upload isn't meant to run mid-frame.
	Platform::Direct3D::GetDefaultBlackTexture();
	Platform::Direct3D::GetDefaultWhiteTexture();
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
		if (ownsTexture[index]) { delete textures[index]; }
		textures[index]    = dynamic_cast<Texture*>(texture);
		ownsTexture[index] = true;
		if (type == TextureType::Albedo)
			params.useAlbedoTexture = (texture != nullptr) ? 1.0f : 0.0f;
		else if (type == TextureType::ARM)
			params.useARMTexture = (texture != nullptr) ? 1.0f : 0.0f;
	}
}

void Material::CopyTexturesFrom(const ZNMaterial* other)
{
	const Material* src = dynamic_cast<const Material*>(other);
	if (!src) return;
	for (size_t i = 0; i < textures.size(); ++i)
	{
		if (!src->textures[i]) continue;
		if (ownsTexture[i]) { delete textures[i]; }
		textures[i]    = src->textures[i];
		ownsTexture[i] = false; // borrowed — source material owns it
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
		paramsToUse.metallic         = 0.0f;
		paramsToUse.roughness        = 1.0f;
		paramsToUse.useAlbedoTexture = 0.0f; // flat color only in wireframe
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

	// Material texture slot -> shader register. Not positional: t3 belongs to the instanced
	// GBuffer's world-matrix buffer and the forward pass's shadow map (both set in Mesh.cpp
	// before this runs), so Emissive takes t4.
	static constexpr SRV_REGISTER kTextureRegister[static_cast<size_t>(TextureType::Count)] = {
		SRV_REGISTER::t0, // Albedo
		SRV_REGISTER::t1, // Normal
		SRV_REGISTER::t2, // ARM
		SRV_REGISTER::t4, // Emissive — free in gbuffer.hlsli / gbuffer_instanced.hlsli
	};

	// Bind textures. Slots with no real texture get the shared black placeholder instead of
	// being skipped — the descriptor table is a rotating per-object pool, so a skipped slot
	// would otherwise keep whatever unrelated texture a prior draw left there.
	for (size_t i = 0; i < textures.size(); ++i)
	{
		// Forward shaders use t4 for IBL irradiance and have no emissive term.
		if (isForwardPass && i == static_cast<size_t>(TextureType::Emissive))
			continue;

		SRV_REGISTER srvRegister = kTextureRegister[i];
		if (i == 0 && hasAlbedoSRVOverride)
			tableDescHeap->SetSRV(albedoSRVOverride, SRV_REGISTER::t0);
		else if (textures[i])
			tableDescHeap->SetSRV(textures[i]->GetCpuHandle(), srvRegister);
		else
		{
			// Missing emissive maps use white so emissiveFactor can work on its own.
			Texture* fallback = i == static_cast<size_t>(TextureType::Emissive)
				? Platform::Direct3D::GetDefaultWhiteTexture()
				: Platform::Direct3D::GetDefaultBlackTexture();
			tableDescHeap->SetSRV(fallback->GetCpuHandle(), srvRegister);
		}
	}
}
