#pragma once
#include "../ZNTransform.h"
#include "../ZNInputDef.h"
#include "../Graphics/ZNGraphicsContext.h" // for DiscoSource
#include "ZNObjectHandle.h"
#include <d3d12.h>
#include <vector>
#include <string>
#include <memory>
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
		// out-of-line: pool holds unique_ptr<ZNGameObject> (needs complete type in .cpp).
		// frees every scene-owned gameobject.
		virtual ~ZNScene();

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

		// R1: creates a mesh-less model root at `modelTransform`, registers it, and returns it.
		// Parent mesh objects under it (root->AddChild(part)) so the model moves as one unit.
		ZNGameObject* AddModelRoot(const std::string& name, const Transform& modelTransform);

		// Add* adopt ownership into the scene's pool (don't delete the object elsewhere) and
		// return a handle. the raw pointer stays valid for the object's lifetime (heap-fixed).
		ZNObjectHandle AddGameObject(ZNGameObject* obj);
		void           RemoveGameObject(ZNGameObject* obj);   // frees the object
		// Ownership enumeration (all live objects of this category), rebuilt on call from
		// objectSlots. NOT the render path — passes iterate the slots directly (see .cpp).
		const std::vector<ZNGameObject*>& GetGameObjects() const;

		// Forward objects (rendered after deferred lighting)
		ZNObjectHandle AddForwardGameObject(ZNGameObject* obj);
		void           RemoveForwardGameObject(ZNGameObject* obj);  // frees the object
		const std::vector<ZNGameObject*>& GetForwardGameObjects() const;

		// Resolve returns null for a stale/null handle instead of a dangling pointer. Destroy
		// frees the object + its child subtree, detaches from parent, drops from render list.
		ZNGameObject* Resolve(ZNObjectHandle h) const;
		bool          IsValid(ZNObjectHandle h) const { return Resolve(h) != nullptr; }
		void          Destroy(ZNObjectHandle h);
		void          Destroy(ZNGameObject* obj);  // no-op if obj isn't pool-owned

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

		// Disco sources — facet-mirror bodies (ball, monster tiles) that scatter the spotlights
		// onto the room. Rebuilt each frame by the owning scene's Update() (rotation changes),
		// pushed to GraphicsContext by Render()/RenderForward() alongside the lights. Empty for
		// scenes that never set it, so nothing leaks between scenes.
		void SetDiscoSources(const std::vector<DiscoSource>& sources) { sceneDiscoSources = sources; }

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

		// Loads a static equirectangular panorama image as the visible background — drawn
		// wherever the depth buffer has no scene geometry (see deferred_lighting.hlsli).
		// Separate from AddCubemapCapture/SetEnvCubemapTexture above, which only feed
		// reflections/IBL and are never drawn directly.
		void SetSkyboxTexture(const std::wstring& panoramaPath, uint32 faceSize = 512);

		// Same re-apply-on-scene-switch pattern as ApplyEnvCubemap(), for the skybox slot.
		void ApplySkybox();

	protected:
		// Enumeration caches (ownership per category), rebuilt on demand by the getters from
		// objectSlots — the single source of truth. The render passes DON'T read these; they
		// iterate objectSlots directly (ForEachLiveObject), so "what's drawn" is always derived
		// from "what exists" and can't desync (no manually-maintained render list -> no ghosts).
		mutable std::vector<ZNGameObject*> gameObjects;
		mutable std::vector<ZNGameObject*> forwardGameObjects;
		std::vector<ZNSpotLight*> spotLights;
		std::vector<ZNPointLight*> pointLights;
		std::vector<DiscoSource> sceneDiscoSources;
		ZNCamera* camera = nullptr;
		ZNDirectionalLight* directionalLight = nullptr;

	private:
		// owns every gameobject. one live slot = one unique_ptr (heap-fixed, so pointers/hierarchy
		// links stay valid across pool growth). generation goes stale on reuse; forward records
		// which render-list view the object is in.
		struct ObjectSlot {
			std::unique_ptr<ZNGameObject> obj;        // null when free
			uint32 generation = 0;                    // bumped on (re)use
			bool   forward    = false;                // forwardGameObjects vs gameObjects
		};
		std::vector<ObjectSlot> objectSlots;
		std::vector<uint32>     freeSlots;            // reusable slot indices

		ZNObjectHandle AdoptObject(ZNGameObject* obj, bool forward);
		void           DestroyObjectInternal(ZNGameObject* obj);

		// Single live-object traversal (one category). Both the render passes and the enumeration
		// caches derive from this, keeping objectSlots the one source of truth. Rebuilt every frame
		// with no dirty-flag caching — cheap at this scale; add caching later only if profiling says.
		template<typename F>
		void ForEachLiveObject(bool forward, F&& action) const
		{
			for (const auto& slot : objectSlots)
				if (slot.obj && slot.forward == forward)
					action(slot.obj.get());
		}
		void RebuildEnumeration(std::vector<ZNGameObject*>& out, bool forward) const;

		// Pushes camera + lights + disco sources to the global GraphicsContext. Called once per
		// frame from Render() (see .cpp) — previously duplicated in Render() and RenderForward().
		void           SyncGraphicsContext();

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

		// Set by SetSkyboxTexture(), applied to CommandQueue only via ApplySkybox().
		D3D12_CPU_DESCRIPTOR_HANDLE ownedSkyboxSRV = {};
		bool hasOwnedSkybox = false;
	};
}
