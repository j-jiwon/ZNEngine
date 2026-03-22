#pragma once
#include "../ZNTransform.h"
#include <vector>

namespace ZNFramework
{
	class ZNGameObject;
	class ZNCamera;
	class ZNLight;
	class ZNDirectionalLight;
	class ZNSpotLight;

	class ZNScene
	{
	public:
		ZNScene() = default;
		virtual ~ZNScene() = default;

		virtual void Initialize() {}
		virtual void Update(float deltaTime);
		virtual void Render();

		// GameObject management
		void AddGameObject(ZNGameObject* obj);
		void RemoveGameObject(ZNGameObject* obj);
		const std::vector<ZNGameObject*>& GetGameObjects() const { return gameObjects; }

		// Camera
		void SetCamera(ZNCamera* cam);
		ZNCamera* GetCamera() const { return camera; }

		// Lighting
		void SetLight(ZNLight* light);
		ZNLight* GetLight() const { return primaryLight; }

		void SetDirectionalLight(ZNDirectionalLight* light);
		ZNDirectionalLight* GetDirectionalLight() const { return directionalLight; }

	protected:
		std::vector<ZNGameObject*> gameObjects;
		ZNCamera* camera = nullptr;
		ZNLight* primaryLight = nullptr;
		ZNDirectionalLight* directionalLight = nullptr;
	};
}
