#pragma once
#include <vector>
#include "../../ZNFramework.h"

namespace ZNFramework
{
	struct Vertex;
	class ZNMesh
	{
	public:
		ZNMesh() = default;
		~ZNMesh() = default;

		virtual void Init(const std::vector<Vertex>& vec) = 0;
		virtual void Render() = 0;
	};
}
