#pragma once
#include "../ZNTransform.h"
#include "../ZNInputDef.h"
#include <vector>

namespace ZNFramework
{
	class ZNGameObject;
	class ZNCamera;
	class ZNLight;
	class ZNDirectionalLight;
	class ZNSpotLight;
	class ZNShader;
	class ZNMatrix4;

	class ZNScene
	{
	public:
		ZNScene() = default;
		virtual ~ZNScene() = default;

		virtual void Initialize() {}
		virtual void Update(float deltaTime);
		virtual void Render();
		virtual void RenderShadow(const ZNMatrix4& lightViewProj, ZNShader* shadowShader);  // Shadow pass
		virtual void RenderForward();  // Forward pass for non-deferred objects (e.g., grid)
		virtual void OnKeyboardEvent(const KeyboardEvent& event) {}

		// GameObject management
		void AddGameObject(ZNGameObject* obj);
		void RemoveGameObject(ZNGameObject* obj);
		const std::vector<ZNGameObject*>& GetGameObjects() const { return gameObjects; }

		// Forward objects (rendered after deferred lighting)
		void AddForwardGameObject(ZNGameObject* obj);
		void RemoveForwardGameObject(ZNGameObject* obj);
		const std::vector<ZNGameObject*>& GetForwardGameObjects() const { return forwardGameObjects; }

		// Camera
		void SetCamera(ZNCamera* cam);
		ZNCamera* GetCamera() const { return camera; }

		// Lighting
		void SetLight(ZNLight* light);
		ZNLight* GetLight() const { return primaryLight; }

		void SetDirectionalLight(ZNDirectionalLight* light);
		ZNDirectionalLight* GetDirectionalLight() const { return directionalLight; }

		ZNGameObject* FindGameObjectWithTag(const std::string& tag);
		ZNGameObject* FindGameObjectWithName(const std::string& name);

	protected:
		std::vector<ZNGameObject*> gameObjects;
		std::vector<ZNGameObject*> forwardGameObjects;  // Objects rendered in forward pass
		ZNCamera* camera = nullptr;
		ZNLight* primaryLight = nullptr;
		ZNDirectionalLight* directionalLight = nullptr;
	};
}
