#pragma once
#include "Graphics/ZNMesh.h"
#include "ZNUtils.h"

using namespace std;

namespace ZNFramework
{
	class Texture;
	class Material;
	class Mesh : public ZNMesh
	{
	public:
		void Init(const vector<Vertex>& vertrexBuffer, const vector<uint32>& indexBuffer) override;
		void Render() override;
		void RenderInstanced(const vector<ZNMatrix4>& worldMatrices) override;
		void RenderForwardInstanced(const vector<ZNMatrix4>& worldMatrices) override;
		void RenderShadow(const ZNMatrix4& lightViewProj, ZNShader* shadowShader) override;
		void RenderShadowInstanced(const ZNMatrix4& lightViewProj, ZNShader* shadowShader,
		                           const vector<ZNMatrix4>& worldMatrices) override;
		void SetTransform(const Transform& t) override { worldMatrix = t.GetWorldMatrix(); }
		void SetWorldMatrix(const ZNMatrix4& world) override { worldMatrix = world; }
		void SetTexture(ZNTexture* inTexture) override;
		void SetMaterial(ZNMaterial* inMaterial) override;

		uint32 GetIndexCount() const override { return indexCount; }
		uint32 GetVertexCount() const override { return vertexCount; }

	private:
		// Fills cbForwardLight (b2) + the shadow map (t3) and IBL irradiance (t4) SRVs from the
		// current GraphicsContext. Identical for the per-object and instanced forward paths, so
		// both Render() and RenderForwardInstanced() call it.
		void BindForwardLightData(class ZNCamera* camera);
		void CreateVertexBuffer(const vector<Vertex>& buffer);
		void CreateIndexBuffer(const vector<uint32>& buffer);

	private:
		ComPtr<ID3D12Resource>		vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW	vertexBufferView = {};
		uint32 vertexCount = 0;

		ComPtr<ID3D12Resource>		indexBuffer;
		D3D12_INDEX_BUFFER_VIEW		indexBufferView = {};
		uint32 indexCount = 0;

		ZNMatrix4 worldMatrix = {};   // already composed (local or hierarchy world)
		Texture* texture = {};
		Material* material = nullptr;
	};
}
