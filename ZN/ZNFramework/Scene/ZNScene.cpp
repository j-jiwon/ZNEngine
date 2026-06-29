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
#include "../Math/ZNMatrix4.h"
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
