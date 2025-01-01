#pragma once
#include <vector>
#include "ZNFramework.h"

namespace ZNFramework
{
	struct Vertex;
	struct Transform;
	class ZNTexture;
	class ZNMesh
	{
	public:
		ZNMesh() = default;
		~ZNMesh() = default;

		virtual void Init(const std::vector<Vertex>& vertexBuffer, const std::vector<uint32>& indexBuffer) = 0;
		virtual void Render() = 0;
		virtual void SetTransform(const Transform& t) = 0;
		virtual void SetTexture(ZNTexture* inTexture) = 0;
	};
}
