#pragma once
#include <vector>
#include "ZNFramework.h"

namespace ZNFramework
{
	struct Vertex;
	struct Transform;
	class ZNTexture;
	class ZNMaterial;
	class ZNShader;
	class ZNMatrix4;
	class ZNMesh
	{
	public:
		ZNMesh() = default;
		virtual ~ZNMesh() = default;  // polymorphic base — deleted through ZNMesh* (releases GPU buffers)

		virtual void Init(const std::vector<Vertex>& vertexBuffer, const std::vector<uint32>& indexBuffer) = 0;
		virtual void Render() = 0;
		// Draws worldMatrices.size() instances of this mesh in a single DrawIndexedInstanced call
		// (GBuffer pass only — see ZNScene::Render()). Default falls back to one Render() per
		// instance for any backend that doesn't override it.
		virtual void RenderInstanced(const std::vector<ZNMatrix4>& worldMatrices) { Render(); }
		virtual void RenderShadow(const ZNMatrix4& lightViewProj, ZNShader* shadowShader) = 0;
		virtual void SetTransform(const Transform& t) = 0;
		virtual void SetWorldMatrix(const ZNMatrix4& world) = 0;  // R1: hierarchy-composed world
		virtual void SetTexture(ZNTexture* inTexture) = 0;
		virtual void SetMaterial(ZNMaterial* inMaterial) = 0;

		virtual uint32 GetIndexCount() const { return 0; }
		virtual uint32 GetVertexCount() const { return 0; }
	};
}
