#pragma once
#include "../ZNTransform.h"
#include "../ZNInputDef.h"
#include <d3d12.h>
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
	class ZNPointLight;
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

		// Debug camera registration — call from scene's Initialize() to get free indicators
		struct DebugCameraEntry { ZNCamera* cam; std::string name; };
		void RegisterDebugCamera(ZNCamera* cam, const std::string& name);
		const std::vector<DebugCameraEntry>& GetDebugCameras() const { return debugCameras; }

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

		void AddPointLight(ZNPointLight* light);
		void RemovePointLight(ZNPointLight* light);
		const std::vector<ZNPointLight*>& GetPointLights() const { return pointLights; }

		ZNGameObject* FindGameObjectWithTag(const std::string& tag);
		ZNGameObject* FindGameObjectWithName(const std::string& name);

		// Registers an offscreen camera that auto-renders all scene gameObjects using
		// their existing material params (metallic, roughness, albedo) through forwardShader.
		// No manual per-object material matching needed.
		void AddOffscreenCamera(ZNCamera* cam, RenderTexture* rt,
		                        const std::string& resourceName, ZNShader* forwardShader);

		// Captures a static environment cubemap from `position` (6 faces, 90 deg FOV) on the
		// first frame only, using each gameObject's own material params through forwardShader
		// (same auto-render approach as AddOffscreenCamera). Objects in `excludeObjects` are
		// skipped (e.g. the reflective object itself, to avoid capturing it from inside).
		// Sets this cubemap as the active environment reflection for the deferred lighting pass.
		void AddCubemapCapture(const ZNVector3& position, float nearZ, float farZ,
		                       uint32 resolution, const std::string& resourceName,
		                       ZNShader* forwardShader,
		                       const std::vector<ZNGameObject*>& excludeObjects = {});

		// Loads a static equirectangular panorama image, resamples it into a cubemap on the
		// CPU, and sets it as the active environment reflection — overrides (or replaces)
		// whatever AddCubemapCapture set, since only one env cubemap SRV is bound at a time.
		void SetEnvCubemapTexture(const std::wstring& panoramaPath, uint32 faceSize = 512);

		// Re-applies this scene's own env cubemap (or clears it, if this scene never called
		// AddCubemapCapture/SetEnvCubemapTexture) to the single global CommandQueue slot.
		// AddCubemapCapture/SetEnvCubemapTexture only record the source at Initialize() time —
		// they don't push it live, since every scene is eagerly Initialize()'d up front
		// (see App.cpp) and would otherwise clobber each other's slot. Call this whenever
		// the active scene changes (ApplicationContext::SetScene() does this automatically).
		void ApplyEnvCubemap();

	protected:
		std::vector<ZNGameObject*> gameObjects;
		std::vector<ZNGameObject*> forwardGameObjects;  // Objects rendered in forward pass
		std::vector<ZNSpotLight*> spotLights;
		std::vector<ZNPointLight*> pointLights;
		ZNCamera* camera = nullptr;
		ZNDirectionalLight* directionalLight = nullptr;

	private:
		std::vector<DebugCameraEntry> debugCameras;

		struct OffscreenCamEntry {
			ZNCamera*    cam;
			RenderTexture* rt;
			std::string  resourceName;
			ZNShader*    forwardShader;
			std::unordered_map<ZNMaterial*, ZNMaterial*> matCache;
		};
		std::vector<OffscreenCamEntry> offscreenCamEntries;

		struct CubemapCaptureEntry {
			ZNShader*    forwardShader;
			std::vector<ZNGameObject*> excludeObjects;
			std::unordered_map<ZNMaterial*, ZNMaterial*> matCache;
		};
		std::vector<CubemapCaptureEntry> cubemapCaptureEntries;

		// Set by AddCubemapCapture/SetEnvCubemapTexture (whichever was called last, if both
		// were), applied to CommandQueue only via ApplyEnvCubemap().
		D3D12_CPU_DESCRIPTOR_HANDLE ownedEnvCubemapSRV = {};
		bool hasOwnedEnvCubemap = false;
	};
}
