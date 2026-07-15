#include "ZNScene.h"
#include "ZNGameObject.h"
#include "../ZNCamera.h"
#include "../Graphics/ZNLight.h"
#include "../Graphics/ZNGraphicsContext.h"
#include "../Graphics/ZNMaterial.h"
#include "../Graphics/ZNMaterialParams.h"
#include "../Graphics/ZNMaterialFactory.h"
#include "../Graphics/Platform/Direct3D12/CommandQueue.h"
#include "../Graphics/Platform/Direct3D12/RenderTexture.h"
#include "../Graphics/Platform/Direct3D12/CubeRenderTexture.h"
#include "../Graphics/Platform/Direct3D12/EquirectCubeTexture.h"
#include "../Graphics/Platform/Direct3D12/SkyboxRenderer.h"
#include "../Math/ZNMatrix4.h"
#include "../Math/ZNVector3.h"
#include <algorithm>

using namespace ZNFramework;

void ZNScene::Update(float deltaTime)
{
	// Update all game objects
	for (auto* obj : gameObjects)
	{
		if (obj)
			obj->Update(deltaTime);
	}

	// Update camera if exists
	if (camera)
	{
		camera->UpdateViewMatrix();
	}
}

void ZNScene::Render()
{
	// Set camera and lights to GraphicsContext
	GraphicsContext& ctx = GraphicsContext::GetInstance();
	ctx.SetCamera(camera);
	ctx.SetSpotLights(spotLights);
	ctx.SetPointLights(pointLights);
	ctx.SetDirectionalLight(directionalLight);
	ctx.SetDiscoSources(sceneDiscoSources);

	// Render all game objects (deferred pass)
	for (auto* obj : gameObjects)
	{
		if (obj)
			obj->Render();
	}
}

void ZNScene::RenderShadow(const ZNMatrix4& lightViewProj, ZNShader* shadowShader)
{
	// Render all game objects for shadow pass
	for (auto* obj : gameObjects)
	{
		if (obj)
			obj->RenderShadow(lightViewProj, shadowShader);
	}
}

void ZNScene::RenderForward()
{
	// Set camera and lights to GraphicsContext (in case they weren't set)
	GraphicsContext& ctx = GraphicsContext::GetInstance();
	ctx.SetCamera(camera);
	ctx.SetSpotLights(spotLights);
	ctx.SetPointLights(pointLights);
	ctx.SetDirectionalLight(directionalLight);
	ctx.SetDiscoSources(sceneDiscoSources);

	// Render forward objects (after deferred lighting)
	for (auto* obj : forwardGameObjects)
	{
		if (obj)
			obj->Render();
	}
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

void ZNScene::AddGameObject(ZNGameObject* obj)
{
	if (obj)
		gameObjects.push_back(obj);
}

void ZNScene::RemoveGameObject(ZNGameObject* obj)
{
	auto it = std::find(gameObjects.begin(), gameObjects.end(), obj);
	if (it != gameObjects.end())
		gameObjects.erase(it);
}

void ZNScene::AddForwardGameObject(ZNGameObject* obj)
{
	if (obj)
		forwardGameObjects.push_back(obj);
}

void ZNScene::RemoveForwardGameObject(ZNGameObject* obj)
{
	auto it = std::find(forwardGameObjects.begin(), forwardGameObjects.end(), obj);
	if (it != forwardGameObjects.end())
		forwardGameObjects.erase(it);
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
	for (auto* obj : gameObjects)
	{
		if (obj && obj->GetTag() == tag)
			return obj;
	}
	return nullptr;
}

ZNGameObject* ZNScene::FindGameObjectWithName(const std::string& name)
{
	for (auto* obj : gameObjects)
	{
		if (obj && obj->GetName() == name)
			return obj;
	}
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
		OffscreenCamEntry& entry = offscreenCamEntries[idx];

		for (auto* obj : gameObjects)
		{
			if (!obj || !obj->IsVisible() || !obj->GetMesh()) continue;

			ZNMaterial* mainMat = obj->GetMaterial();
			if (!mainMat) continue;

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
		}
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

		// Fill the whole face with the active skybox first (if any), so directions with
		// no scene geometry show sky instead of the render target's black clear color.
		// Real geometry rendered below naturally overwrites it via depth test.
		CommandQueue* cmdQ2 = GraphicsContext::GetInstance().GetAs<CommandQueue>();
		SkyboxRenderer* skyboxRenderer = cmdQ2->GetSkyboxRenderer();
		if (skyboxRenderer)
		{
			ID3D12GraphicsCommandList* cmd = cmdQ2->CommandList();
			skyboxRenderer->DrawBackground(cmd, face, cmdQ2->HasSkybox(), cmdQ2->GetSkyboxSRV(),
			                               cubeRT->GetRTV(face), resolution);
		}

		// CubeCapturePass executes before GBufferPass in the render graph, which is the
		// only place ZNScene::Render() (and thus SetSpotLights/SetDirectionalLight) normally
		// runs. Since this capture is one-shot on the very first frame, GraphicsContext's
		// light data would otherwise still be unset — so set it explicitly here.
		GraphicsContext& ctx = GraphicsContext::GetInstance();
		ctx.SetSpotLights(spotLights);
		ctx.SetPointLights(pointLights);
		ctx.SetDirectionalLight(directionalLight);

		for (auto* obj : gameObjects)
		{
			if (!obj || !obj->IsVisible() || !obj->GetMesh()) continue;
			if (std::find(entry.excludeObjects.begin(), entry.excludeObjects.end(), obj) != entry.excludeObjects.end())
				continue;

			ZNMaterial* mainMat = obj->GetMaterial();
			if (!mainMat) continue;

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
		}
	});

	ownedEnvCubemapSRV = cubeRT->GetSRVCpuHandle();
	hasOwnedEnvCubemap = true;
}

void ZNScene::SetEnvCubemapTexture(const std::wstring& panoramaPath, uint32 faceSize)
{
	auto* cubeTex = new EquirectCubeTexture();
	cubeTex->Init(panoramaPath, faceSize);

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
	cubeTex->Init(panoramaPath, faceSize);

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
