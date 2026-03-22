#include "ZNScene.h"
#include "ZNGameObject.h"
#include "../ZNCamera.h"
#include "../Graphics/ZNLight.h"
#include "../Graphics/ZNGraphicsContext.h"
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

		// Update spot light to follow camera if it's a flashlight
		if (primaryLight && primaryLight->GetType() == LightType::Spot)
		{
			ZNSpotLight* spotLight = static_cast<ZNSpotLight*>(primaryLight);
			spotLight->SetPosition(camera->GetPosition());

			// Calculate forward direction from pitch and yaw
			float pitch = camera->GetPitch();
			float yaw = camera->GetYaw();
			ZNVector3 forward;
			forward.x = cos(pitch) * sin(yaw);
			forward.y = sin(pitch);
			forward.z = cos(pitch) * cos(yaw);
			spotLight->SetDirection(forward);
		}
	}
}

void ZNScene::Render()
{
	// Set camera and lights to GraphicsContext
	GraphicsContext& ctx = GraphicsContext::GetInstance();
	ctx.SetCamera(camera);
	ctx.SetLight(primaryLight);
	ctx.SetDirectionalLight(directionalLight);

	// Render all game objects
	for (auto* obj : gameObjects)
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

void ZNScene::SetCamera(ZNCamera* cam)
{
	camera = cam;
}

void ZNScene::SetLight(ZNLight* light)
{
	primaryLight = light;
}

void ZNScene::SetDirectionalLight(ZNDirectionalLight* light)
{
	directionalLight = light;
}
