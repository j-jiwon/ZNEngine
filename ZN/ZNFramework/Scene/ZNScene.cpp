#include "ZNScene.h"
#include "ZNGameObject.h"
#include "../ZNCamera.h"
#include "../Graphics/ZNLight.h"
#include "../Graphics/ZNGraphicsContext.h"
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
	ctx.SetDirectionalLight(directionalLight);

	// Render forward objects (after deferred lighting)
	for (auto* obj : forwardGameObjects)
	{
		if (obj)
			obj->Render();
	}
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
