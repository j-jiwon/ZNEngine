#pragma once
#include "Graphics/ZNMesh.h"
#include "ZNUtils.h"

using namespace std;

namespace ZNFramework
{
	class Texture;
	class Mesh : public ZNMesh
	{
	public:
		void Init(const vector<Vertex>& vertrexBuffer, const vector<uint32>& indexBuffer) override;
		void Render() override;
		void SetTransform(const Transform& t) override { transform = t; }
		void SetTexture(ZNTexture* inTexture) override;

	private:
		void CreateVertexBuffer(const vector<Vertex>& buffer);
		void CreateIndexBuffer(const vector<uint32>& buffer);

	private:
		ComPtr<ID3D12Resource>		vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW	vertexBufferView = {};
		uint32 vertexCount = 0;

		ComPtr<ID3D12Resource>		indexBuffer;
		D3D12_INDEX_BUFFER_VIEW		indexBufferView = {};
		uint32 indexCount = 0;

		Transform transform = {};
		Texture* texture = {};
	};
}
