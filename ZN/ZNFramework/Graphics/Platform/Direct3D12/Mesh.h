#pragma once
#include "ZNUtils.h"
#include "../../ZNMesh.h"

using namespace std;

namespace ZNFramework
{
	class Mesh : public ZNMesh
	{
	public:
		void Init(const std::vector<Vertex>& vec) override;
		void Render() override;

	private:
		ComPtr<ID3D12Resource>		vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW	vertexBufferView = {};
		UINT vertexCount = 0;
	};
}
