#pragma once
#include <string>
#include "../ZNTransform.h"

namespace ZNFramework
{
	class ZNMesh;
	class ZNMaterial;
	class ZNShader;
	class ZNMatrix4;

	class ZNGameObject
	{
	public:
		ZNGameObject() = default;
		virtual ~ZNGameObject() = default;

		virtual void Update(float deltaTime) {}
		virtual void Render();
		virtual void RenderShadow(const ZNMatrix4& lightViewProj, ZNShader* shadowShader);

		void SetMesh(ZNMesh* inMesh) { mesh = inMesh; }
		void SetMaterial(ZNMaterial* inMaterial) { material = inMaterial; }
		void SetActive(bool active) { isActive = active; }
		bool IsActive() const { return isActive; }
		void SetVisible(bool visible) { isVisible = visible; }
		bool IsVisible() const { return isVisible; }
		void SetCastShadow(bool value) { castShadow = value; }

		ZNMesh* GetMesh() const { return mesh; }
		ZNMaterial* GetMaterial() const { return material; }

		static void FlushDrawCalls() { sLastFrameDrawCalls = sDrawCalls; sDrawCalls = 0; }
		static int GetLastFrameDrawCalls() { return sLastFrameDrawCalls; }

	private:
		static int sDrawCalls;
		static int sLastFrameDrawCalls;

	public:

		Transform& GetTransform() { return transform; }
		const Transform& GetTransform() const { return transform; }

		std::string GetName() const { return name; }
		void SetName(const std::string& newName) { name = newName; }
		std::string GetTag() const { return tag; }
		void SetTag(const std::string& newTag) { tag = newTag; }	

	protected:
		ZNMesh* mesh = nullptr;
		ZNMaterial* material = nullptr;
		Transform transform;
		bool isActive = true;
		bool isVisible = true;
		bool castShadow = true;
		std::string name;
		std::string tag;
	};
}
