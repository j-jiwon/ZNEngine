#include "ZNScene.h"
#include "ZNGameObject.h"
#include "../ZNCamera.h"
#include "../Graphics/ZNLight.h"
#include "../Graphics/ZNGraphicsContext.h"
#include "../Graphics/ZNMaterial.h"
#include "../Graphics/ZNMaterialParams.h"
#include "../Graphics/ZNMaterialFactory.h"
#include "../Graphics/ZNMesh.h"
#include "../Graphics/ZNShader.h"
#include "../Graphics/Platform/Direct3D12/CommandQueue.h"
#include "../Graphics/Platform/Direct3D12/RenderTexture.h"
#include "../Graphics/Platform/Direct3D12/CubeRenderTexture.h"
#include "../Graphics/Platform/Direct3D12/EquirectCubeTexture.h"
#include "../Graphics/Platform/Direct3D12/SkyboxRenderer.h"
#include "../Math/ZNMatrix4.h"
#include "../Math/ZNVector3.h"
#include <algorithm>

using namespace ZNFramework;

ZNScene* ZNScene::s_activeScene = nullptr;

// out-of-line: unique_ptr<ZNGameObject> needs the complete type here. frees every gameobject.
ZNScene::~ZNScene() = default;

void ZNScene::Update(float deltaTime)
{
	// Update every live object (both categories), straight from the source of truth.
	for (const auto& slot : objectSlots)
	{
		if (slot.obj)
			slot.obj->Update(deltaTime);
	}

	// Update camera if exists
	if (camera)
	{
		camera->UpdateViewMatrix();
	}
}

void ZNScene::SyncGraphicsContext()
{
	GraphicsContext& ctx = GraphicsContext::GetInstance();
	ctx.SetCamera(camera);
	ctx.SetSpotLights(spotLights);
	ctx.SetPointLights(pointLights);
	ctx.SetDirectionalLight(directionalLight);
	ctx.SetDiscoSources(sceneDiscoSources);
}

void ZNScene::Render()
{
	// Runs inside GBufferPass, before DeferredLightingPass/ForwardRenderPass read the context.
	SyncGraphicsContext();

	GraphicsContext& ctx = GraphicsContext::GetInstance();
	ZNCommandQueue* cq = ctx.GetCommandQueue();
	ZNShader* instancedShader = ctx.GetGBufferInstancedShader();

	// Wireframe's per-object selection highlight can't be expressed by a shared instanced draw
	// (one draw = one material/CB for every instance in it), so skip batching in that view mode.
	if (!instancedShader || (cq && cq->GetViewMode() == ViewMode::Wireframe))
	{
		ForEachLiveObject(false, [](ZNGameObject* obj) { obj->Render(); });
		return;
	}

	// Group opaque objects that share a Mesh (already 1:1 with Material in this codebase — see
	// SceneBinding::Apply / VehicleScene::SpawnCarInstance, both hand every instance the SAME
	// shared Mesh*) into a single instanced draw each; anything with a unique Mesh (Ground, EGO,
	// other scenes' one-off objects) falls back to the normal per-object path unchanged.
	std::unordered_map<ZNMesh*, std::vector<ZNGameObject*>> byMesh;
	ForEachLiveObject(false, [&](ZNGameObject* obj) {
		if (obj->IsVisible() && obj->GetMesh())
			byMesh[obj->GetMesh()].push_back(obj);
	});

	std::vector<std::pair<ZNMesh*, std::vector<ZNGameObject*>>> batched;
	std::vector<ZNGameObject*> singles;
	for (auto& [meshPtr, objs] : byMesh)
	{
		if (objs.size() >= 2)
			batched.emplace_back(meshPtr, std::move(objs));
		else
			for (ZNGameObject* obj : objs) singles.push_back(obj);
	}

	// Draw every batch first, all under one PSO bind...
	if (!batched.empty())
	{
		instancedShader->Bind();

		std::vector<ZNMatrix4> worldMatrices;
		for (auto& [meshPtr, objs] : batched)
		{
			worldMatrices.clear();
			worldMatrices.reserve(objs.size());
			for (ZNGameObject* obj : objs)
				worldMatrices.push_back(obj->GetWorldMatrix());

			meshPtr->RenderInstanced(worldMatrices);

			ZNGameObject::RecordDrawCall(
				static_cast<int>(meshPtr->GetIndexCount() / 3) * static_cast<int>(objs.size()),
				static_cast<int>(meshPtr->GetVertexCount()) * static_cast<int>(objs.size()));
		}
	}

	// ...then restore the normal GBuffer PSO before the per-object path, which (via
	// Material::Bind()) assumes it's already bound and never rebinds it itself in MRT mode.
	if (!singles.empty())
	{
		if (!batched.empty())
		{
			ZNShader* normalShader = ctx.GetGBufferShader();
			if (normalShader) normalShader->Bind();
		}
		for (ZNGameObject* obj : singles)
			obj->Render();
	}
}

void ZNScene::RenderShadow(const ZNMatrix4& lightViewProj, ZNShader* shadowShader)
{
	// Shadow casters come from the deferred (opaque) list; each object self-filters on
	// castShadow. Forward/transparent objects (glass, windows) intentionally cast no shadow.
	ForEachLiveObject(false, [&](ZNGameObject* obj) { obj->RenderShadow(lightViewProj, shadowShader); });
}

void ZNScene::RenderForward()
{
	// GraphicsContext was already synced by Render() earlier this frame (GBufferPass precedes
	// ForwardRenderPass, and nothing in between rebinds camera/lights), so no re-push here.
	ForEachLiveObject(true, [](ZNGameObject* obj) { obj->Render(); });
}

void ZNScene::RegisterDebugCamera(ZNCamera* cam, const std::string& name)
{
	if (cam)
		debugCameras.push_back({ cam, name });
}

ZNGameObject* ZNScene::AddModelRoot(const std::string& name, const Transform& modelTransform)
{
	ZNGameObject* root = new ZNGameObject();
	root->SetName(name);
	root->GetTransform() = modelTransform;
	// No mesh -> renders nothing; it only drives child world transforms + Outliner grouping.
	AddGameObject(root);
	return root;
}

// --- object pool -------------------------------------------------------------------------

ZNObjectHandle ZNScene::AdoptObject(ZNGameObject* obj, bool forward)
{
	if (!obj)
		return {};

	uint32 index;
	if (!freeSlots.empty())
	{
		index = freeSlots.back();
		freeSlots.pop_back();
	}
	else
	{
		index = static_cast<uint32>(objectSlots.size());
		objectSlots.emplace_back();
	}

	ObjectSlot& slot = objectSlots[index];
	slot.generation += 1;          // bumped every use -> old handles go stale
	slot.forward = forward;
	slot.obj.reset(obj);           // take ownership

	ZNObjectHandle h{ index, slot.generation };
	obj->SetHandle(h);
	return h;
}

ZNGameObject* ZNScene::Resolve(ZNObjectHandle h) const
{
	if (h.IsNull() || h.index >= objectSlots.size())
		return nullptr;
	const ObjectSlot& slot = objectSlots[h.index];
	if (slot.generation != h.generation || !slot.obj)
		return nullptr;            // stale: slot freed or reused
	return slot.obj.get();
}

void ZNScene::RebuildEnumeration(std::vector<ZNGameObject*>& out, bool forward) const
{
	// Ownership enumeration, derived on demand from objectSlots (the source of truth). Called a
	// handful of times per frame by the UI (Stats/Outliner) — O(pool capacity), negligible here.
	out.clear();
	ForEachLiveObject(forward, [&](ZNGameObject* obj) { out.push_back(obj); });
}

void ZNScene::DestroyObjectInternal(ZNGameObject* obj)
{
	// destroy the subtree first (children are separately pool-owned). copy the list because
	// each child's DetachFromParent() mutates it.
	std::vector<ZNGameObject*> kids = obj->GetChildren();
	for (auto* child : kids)
		DestroyObjectInternal(child);

	obj->DetachFromParent();  // so parent's children list doesn't dangle

	const ZNObjectHandle h = obj->GetHandle();
	ObjectSlot& slot = objectSlots[h.index];
	slot.obj.reset();          // free; render passes derive from slots, so it drops out immediately
	freeSlots.push_back(h.index);
}

void ZNScene::Destroy(ZNObjectHandle h)
{
	if (ZNGameObject* obj = Resolve(h))
		DestroyObjectInternal(obj);
}

void ZNScene::Destroy(ZNGameObject* obj)
{
	// only if obj is genuinely a live pool object (guards double-destroy / non-adopted ptr).
	if (obj && Resolve(obj->GetHandle()) == obj)
		DestroyObjectInternal(obj);
}

ZNObjectHandle ZNScene::AddGameObject(ZNGameObject* obj)
{
	if (!obj)
		return {};
	// Adopt into the slot pool only; the render list is derived from slots, not maintained here.
	return AdoptObject(obj, /*forward*/ false);
}

void ZNScene::RemoveGameObject(ZNGameObject* obj)
{
	Destroy(obj);
}

ZNObjectHandle ZNScene::AddForwardGameObject(ZNGameObject* obj)
{
	if (!obj)
		return {};
	return AdoptObject(obj, /*forward*/ true);
}

void ZNScene::RemoveForwardGameObject(ZNGameObject* obj)
{
	Destroy(obj);
}

const std::vector<ZNGameObject*>& ZNScene::GetGameObjects() const
{
	RebuildEnumeration(gameObjects, /*forward*/ false);
	return gameObjects;
}

const std::vector<ZNGameObject*>& ZNScene::GetForwardGameObjects() const
{
	RebuildEnumeration(forwardGameObjects, /*forward*/ true);
	return forwardGameObjects;
}

void ZNScene::SetCamera(ZNCamera* cam)
{
	camera = cam;
}

void ZNScene::AddSpotLight(ZNSpotLight* light)
{
	if (light)
		spotLights.push_back(light);
}

void ZNScene::RemoveSpotLight(ZNSpotLight* light)
{
	auto it = std::find(spotLights.begin(), spotLights.end(), light);
	if (it != spotLights.end())
		spotLights.erase(it);
}

void ZNScene::SetDirectionalLight(ZNDirectionalLight* light)
{
	directionalLight = light;
}

void ZNScene::AddPointLight(ZNPointLight* light)
{
	if (light)
		pointLights.push_back(light);
}

void ZNScene::RemovePointLight(ZNPointLight* light)
{
	auto it = std::find(pointLights.begin(), pointLights.end(), light);
	if (it != pointLights.end())
		pointLights.erase(it);
}

ZNGameObject* ZNScene::FindGameObjectWithTag(const std::string& tag)
{
	// Search all owned objects (both categories), straight from the source of truth.
	for (const auto& slot : objectSlots)
		if (slot.obj && slot.obj->GetTag() == tag)
			return slot.obj.get();
	return nullptr;
}

ZNGameObject* ZNScene::FindGameObjectWithName(const std::string& name)
{
	for (const auto& slot : objectSlots)
		if (slot.obj && slot.obj->GetName() == name)
			return slot.obj.get();
	return nullptr;
}

void ZNScene::AddOffscreenCamera(ZNCamera* cam, RenderTexture* rt,
                                  const std::string& resourceName, ZNShader* forwardShader)
{
	offscreenCamEntries.push_back({ cam, rt, resourceName, forwardShader, {} });
	const size_t idx = offscreenCamEntries.size() - 1;

	CommandQueue* cmdQ = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	cmdQ->AddOffscreenCamera(cam, rt, resourceName, [this, idx]()
	{
		// Every scene's offscreen passes stay registered (eager init); skip the geometry work
		// unless this scene is the active one. The RT keeps its last frame, which is never shown
		// while the scene is inactive (each scene draws only its own thumbnails).
		if (!IsActiveScene()) return;

		OffscreenCamEntry& entry = offscreenCamEntries[idx];

		ForEachLiveObject(false, [&](ZNGameObject* obj)
		{
			if (!obj->IsVisible() || !obj->GetMesh()) return;

			ZNMaterial* mainMat = obj->GetMaterial();
			if (!mainMat) return;

			// Lookup or lazily create the forward material for this source material.
			ZNMaterial* fwdMat = nullptr;
			auto it = entry.matCache.find(mainMat);
			if (it == entry.matCache.end())
			{
				MaterialParams p = mainMat->GetParams();
				fwdMat = ZNMaterialFactory::CreatePBR(
					entry.forwardShader, p.albedoColor, p.metallic, p.roughness, p.ao);
				fwdMat->SetParams(p);        // sync useAlbedoTexture and other fields
				fwdMat->CopyTexturesFrom(mainMat);
				entry.matCache[mainMat] = fwdMat;
			}
			else
			{
				fwdMat = it->second;
				// Sync params so Inspector edits to the main material are reflected.
				fwdMat->SetParams(mainMat->GetParams());
			}

			// Temporarily override the mesh material, render, then restore.
			ZNMaterial* origMat = obj->GetMaterial();
			obj->GetMesh()->SetMaterial(fwdMat);
			obj->Render();
			obj->GetMesh()->SetMaterial(origMat);
		});
	});
}

void ZNScene::AddCubemapCapture(const ZNVector3& position, float nearZ, float farZ,
                                 uint32 resolution, const std::string& resourceName,
                                 ZNShader* forwardShader,
                                 const std::vector<ZNGameObject*>& excludeObjects)
{
	cubemapCaptureEntries.push_back({ forwardShader, excludeObjects, {} });
	const size_t idx = cubemapCaptureEntries.size() - 1;

	auto* cubeRT = new CubeRenderTexture();
	cubeRT->Init(resolution);

	// Standard cubemap face basis (Y-up, left-handed): direction to look + up vector per face.
	static const struct { ZNVector3 dir; ZNVector3 up; } kFaces[6] = {
		{ ZNVector3( 1.f,  0.f,  0.f), ZNVector3(0.f, 1.f,  0.f) }, // +X
		{ ZNVector3(-1.f,  0.f,  0.f), ZNVector3(0.f, 1.f,  0.f) }, // -X
		{ ZNVector3( 0.f,  1.f,  0.f), ZNVector3(0.f, 0.f, -1.f) }, // +Y
		{ ZNVector3( 0.f, -1.f,  0.f), ZNVector3(0.f, 0.f,  1.f) }, // -Y
		{ ZNVector3( 0.f,  0.f,  1.f), ZNVector3(0.f, 1.f,  0.f) }, // +Z
		{ ZNVector3( 0.f,  0.f, -1.f), ZNVector3(0.f, 1.f,  0.f) }, // -Z
	};

	std::vector<ZNCamera*> cams;
	for (int i = 0; i < 6; ++i)
	{
		ZNCamera* cam = new ZNCamera();
		cam->SetPosition(position);
		cam->SetView(position, position + kFaces[i].dir, kFaces[i].up);
		cam->SetPerspective(3.14159265f / 2.0f, 1.0f, nearZ, farZ); // 90 deg FOV, square aspect
		cams.push_back(cam);
	}

	CommandQueue* cmdQ = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	cmdQ->AddCubemapCapture(cams, cubeRT, resourceName, [this, idx, cubeRT, resolution](uint32 face)
	{
		CubemapCaptureEntry& entry = cubemapCaptureEntries[idx];

		// Fill the whole face with THIS scene's own skybox first (if any), so directions with no
		// scene geometry show sky instead of the render target's black clear color. Use the scene's
		// own skybox — not the globally-active one — because this one-shot capture runs on the first
		// frame regardless of which scene is currently active, so the global skybox may belong to a
		// different scene (that leak is what made MirrorBall's reflection pick up another scene's
		// background). Real geometry rendered below naturally overwrites it via depth test.
		CommandQueue* cmdQ2 = GraphicsContext::GetInstance().GetAs<CommandQueue>();
		SkyboxRenderer* skyboxRenderer = cmdQ2->GetSkyboxRenderer();
		if (skyboxRenderer)
		{
			ID3D12GraphicsCommandList* cmd = cmdQ2->CommandList();
			skyboxRenderer->DrawBackground(cmd, face, hasOwnedSkybox, ownedSkyboxSRV,
			                               cubeRT->GetRTV(face), resolution);
		}

		// DrawBackground above re-bound the face RTV with NO depth buffer. Restore RTV + DSV so the
		// geometry below actually depth-tests — otherwise it renders in draw order (wrong occlusion
		// in the captured reflection) and its D32_FLOAT PSO mismatches the null DSV (debug-layer spam).
		{
			ID3D12GraphicsCommandList* cmd = cmdQ2->CommandList();
			D3D12_CPU_DESCRIPTOR_HANDLE rtv = cubeRT->GetRTV(face);
			D3D12_CPU_DESCRIPTOR_HANDLE dsv = cubeRT->GetDSV();
			cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
		}

		// CubeCapturePass executes before GBufferPass in the render graph, which is the
		// only place ZNScene::Render() (and thus SetSpotLights/SetDirectionalLight) normally
		// runs. Since this capture is one-shot on the very first frame, GraphicsContext's
		// light data would otherwise still be unset — so set it explicitly here.
		GraphicsContext& ctx = GraphicsContext::GetInstance();
		ctx.SetSpotLights(spotLights);
		ctx.SetPointLights(pointLights);
		ctx.SetDirectionalLight(directionalLight);

		ForEachLiveObject(false, [&](ZNGameObject* obj)
		{
			if (!obj->IsVisible() || !obj->GetMesh()) return;
			if (std::find(entry.excludeObjects.begin(), entry.excludeObjects.end(), obj) != entry.excludeObjects.end())
				return;

			ZNMaterial* mainMat = obj->GetMaterial();
			if (!mainMat) return;

			ZNMaterial* fwdMat = nullptr;
			auto it = entry.matCache.find(mainMat);
			if (it == entry.matCache.end())
			{
				MaterialParams p = mainMat->GetParams();
				fwdMat = ZNMaterialFactory::CreatePBR(
					entry.forwardShader, p.albedoColor, p.metallic, p.roughness, p.ao);
				fwdMat->SetParams(p);
				fwdMat->CopyTexturesFrom(mainMat);
				entry.matCache[mainMat] = fwdMat;
			}
			else
			{
				fwdMat = it->second;
			}

			ZNMaterial* origMat = obj->GetMaterial();
			obj->GetMesh()->SetMaterial(fwdMat);
			obj->Render();
			obj->GetMesh()->SetMaterial(origMat);
		});
	});

	ownedEnvCubemapSRV = cubeRT->GetSRVCpuHandle();
	hasOwnedEnvCubemap = true;
}

void ZNScene::SetEnvCubemapTexture(const std::wstring& panoramaPath, uint32 faceSize)
{
	auto* cubeTex = new EquirectCubeTexture();
	cubeTex->Init(panoramaPath, faceSize, true);

	ownedEnvCubemapSRV = cubeTex->GetSRVCpuHandle();
	hasOwnedEnvCubemap = true;
}

void ZNScene::ApplyEnvCubemap()
{
	CommandQueue* cmdQ = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	if (hasOwnedEnvCubemap)
		cmdQ->SetEnvCubemapSRV(ownedEnvCubemapSRV);
	else
		cmdQ->ClearEnvCubemapSRV();
}

void ZNScene::SetSkyboxTexture(const std::wstring& panoramaPath, uint32 faceSize)
{
	auto* cubeTex = new EquirectCubeTexture();
	cubeTex->Init(panoramaPath, faceSize, true);

	ownedSkyboxSRV = cubeTex->GetSRVCpuHandle();
	hasOwnedSkybox = true;
}

void ZNScene::ApplySkybox()
{
	CommandQueue* cmdQ = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	if (hasOwnedSkybox)
		cmdQ->SetSkyboxSRV(ownedSkyboxSRV);
	else
		cmdQ->ClearSkyboxSRV();
}
