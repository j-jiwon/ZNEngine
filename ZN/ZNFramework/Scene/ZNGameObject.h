#pragma once
#include <string>
#include <vector>
#include "../ZNTransform.h"
#include "ZNObjectHandle.h"

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

		static void FlushDrawCalls()
		{
			sLastFrameDrawCalls = sDrawCalls; sDrawCalls = 0;
			sLastFrameTriangles = sTriangles; sTriangles = 0;
			sLastFrameVertices  = sVertices;  sVertices  = 0;
		}
		static int GetLastFrameDrawCalls() { return sLastFrameDrawCalls; }
		static int GetLastFrameTriangles()  { return sLastFrameTriangles; }
		static int GetLastFrameVertices()   { return sLastFrameVertices; }

	private:
		static int sDrawCalls;
		static int sLastFrameDrawCalls;
		static int sTriangles;
		static int sLastFrameTriangles;
		static int sVertices;
		static int sLastFrameVertices;

	public:

		Transform& GetTransform() { return transform; }
		const Transform& GetTransform() const { return transform; }

		// --- Hierarchy (R1) ---
		// A "model" = one root node (usually mesh-less) with N mesh children. Moving the root
		// moves every child, because child world = childLocal * parentWorld (see .cpp).
		ZNGameObject* GetParent() const { return parent; }
		const std::vector<ZNGameObject*>& GetChildren() const { return children; }
		bool HasChildren() const { return !children.empty(); }
		bool IsRootLevel() const { return parent == nullptr; }

		void AddChild(ZNGameObject* child);   // re-parents child under this node
		void DetachFromParent();

		// Local = this node's own Transform. World = local composed up the parent chain.
		ZNMatrix4 GetLocalMatrix() const;
		ZNMatrix4 GetWorldMatrix() const;

		std::string GetName() const { return name; }
		void SetName(const std::string& newName) { name = newName; }
		std::string GetTag() const { return tag; }
		void SetTag(const std::string& newTag) { tag = newTag; }

		// stable handle into the owning scene's pool, set when the scene adopts this object.
		ZNObjectHandle GetHandle() const { return handle; }
		void SetHandle(ZNObjectHandle h) { handle = h; }  // scene only

	protected:
		ZNMesh* mesh = nullptr;
		ZNMaterial* material = nullptr;
		Transform transform;             // local transform (relative to parent)
		ZNGameObject* parent = nullptr;  // R1: scene-graph hierarchy
		std::vector<ZNGameObject*> children;
		bool isActive = true;
		bool isVisible = true;
		bool castShadow = true;
		std::string name;
		std::string tag;
		ZNObjectHandle handle;           // identity in the owning scene's pool
	};
}
