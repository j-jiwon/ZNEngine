#pragma once
#include "../ZNTransform.h"
#include "../ZNInputDef.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace ZNFramework
{
	class ZNGameObject;
	class ZNCamera;
	class ZNLight;
	class ZNDirectionalLight;
	class ZNSpotLight;
	class ZNShader;
	class ZNMatrix4;
	class ZNMaterial;
	class RenderTexture;

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
		void AddSpotLight(ZNSpotLight* light);
		void RemoveSpotLight(ZNSpotLight* light);
		const std::vector<ZNSpotLight*>& GetSpotLights() const { return spotLights; }

		void SetDirectionalLight(ZNDirectionalLight* light);
		ZNDirectionalLight* GetDirectionalLight() const { return directionalLight; }

		ZNGameObject* FindGameObjectWithTag(const std::string& tag);
		ZNGameObject* FindGameObjectWithName(const std::string& name);

		// Registers an offscreen camera that auto-renders all scene gameObjects using
		// their existing material params (metallic, roughness, albedo) through forwardShader.
		// No manual per-object material matching needed.
		void AddOffscreenCamera(ZNCamera* cam, RenderTexture* rt,
		                        const std::string& resourceName, ZNShader* forwardShader);

	protected:
		std::vector<ZNGameObject*> gameObjects;
		std::vector<ZNGameObject*> forwardGameObjects;  // Objects rendered in forward pass
		std::vector<ZNSpotLight*> spotLights;
		ZNCamera* camera = nullptr;
		ZNDirectionalLight* directionalLight = nullptr;

	private:
		struct OffscreenCamEntry {
			ZNCamera*    cam;
			RenderTexture* rt;
			std::string  resourceName;
			ZNShader*    forwardShader;
			std::unordered_map<ZNMaterial*, ZNMaterial*> matCache;
		};
		std::vector<OffscreenCamEntry> offscreenCamEntries;
	};
}
