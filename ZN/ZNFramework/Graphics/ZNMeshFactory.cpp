#include "ZNMeshFactory.h"
#include "ZNMesh.h"
#include "Platform/GraphicsAPI.h"
#include <cmath>

namespace ZNFramework
{
	ZNMesh* ZNMeshFactory::CreateCube(float size)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32> indices;
		ZNVector4 color(1, 1, 1, 1);

		auto addFace = [&](ZNVector3 p0, ZNVector3 p1, ZNVector3 p2, ZNVector3 p3, ZNVector3 normal)
		{
			uint32 base = static_cast<uint32>(vertices.size());
			vertices.push_back(Vertex(p0 * size, color, ZNVector2(0, 0), normal));
			vertices.push_back(Vertex(p1 * size, color, ZNVector2(1, 0), normal));
			vertices.push_back(Vertex(p2 * size, color, ZNVector2(1, 1), normal));
			vertices.push_back(Vertex(p3 * size, color, ZNVector2(0, 1), normal));
			indices.insert(indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
		};

		// +Y (top)
		addFace({ -1, 1, -1 }, { 1, 1, -1 }, { 1, 1, 1 }, { -1, 1, 1 }, { 0, 1, 0 });
		// -Y (bottom)
		addFace({ -1, -1, 1 }, { 1, -1, 1 }, { 1, -1, -1 }, { -1, -1, -1 }, { 0, -1, 0 });
		// +Z (front)
		addFace({ -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 }, { 0, 0, 1 });
		// -Z (back)
		addFace({ 1, -1, -1 }, { -1, -1, -1 }, { -1, 1, -1 }, { 1, 1, -1 }, { 0, 0, -1 });
		// +X (right)
		addFace({ 1, -1, 1 }, { 1, -1, -1 }, { 1, 1, -1 }, { 1, 1, 1 }, { 1, 0, 0 });
		// -X (left)
		addFace({ -1, -1, -1 }, { -1, -1, 1 }, { -1, 1, 1 }, { -1, 1, -1 }, { -1, 0, 0 });

		ZNMesh* mesh = Platform::CreateMesh();
		mesh->Init(vertices, indices);
		return mesh;
	}

	ZNMesh* ZNMeshFactory::CreateSphere(float radius, int stacks, int slices)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32> indices;

		const float PI = 3.14159265f;

		for (int i = 0; i <= stacks; ++i)
		{
			float phi = PI * i / stacks; // 0 ~ PI
			for (int j = 0; j <= slices; ++j)
			{
				float theta = 2.0f * PI * j / slices; // 0 ~ 2PI
				float x = radius * sinf(phi) * cosf(theta);
				float y = radius * cosf(phi);
				float z = radius * sinf(phi) * sinf(theta);

				ZNVector3 pos(x, y, z);
				ZNVector3 normal = pos * (1.0f / radius); // Normalized position
				ZNVector2 uv(static_cast<float>(j) / slices, static_cast<float>(i) / stacks);
				vertices.push_back(Vertex(pos, ZNVector4(1, 1, 1, 1), uv, normal));
			}
		}

		for (int i = 0; i < stacks; ++i)
		{
			for (int j = 0; j < slices; ++j)
			{
				uint32 a = i * (slices + 1) + j;
				uint32 b = a + 1;
				uint32 c = a + (slices + 1);
				uint32 d = c + 1;
				indices.insert(indices.end(), { a, c, b, b, c, d });
			}
		}

		ZNMesh* mesh = Platform::CreateMesh();
		mesh->Init(vertices, indices);
		return mesh;
	}

	ZNMesh* ZNMeshFactory::CreateCone(float radius, float height, int slices)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32> indices;

		const float PI = 3.14159265f;
		ZNVector4 color(1, 1, 1, 1);

		// Apex vertex (top of cone)
		ZNVector3 apex(0.0f, height, 0.0f);

		// Calculate slope for normal
		float slope = radius / height;

		// Side vertices
		uint32 apexIndex = 0;
		vertices.push_back(Vertex(apex, color, ZNVector2(0.5f, 0.0f), ZNVector3(0, 1, 0)));

		// Base circle vertices for sides
		for (int i = 0; i <= slices; ++i)
		{
			float theta = 2.0f * PI * i / slices;
			float x = radius * cosf(theta);
			float z = radius * sinf(theta);

			ZNVector3 pos(x, 0.0f, z);
			// Normal points outward and upward along the cone surface
			ZNVector3 normal(cosf(theta), slope, sinf(theta));
			float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
			normal = normal * (1.0f / len);

			ZNVector2 uv(static_cast<float>(i) / slices, 1.0f);
			vertices.push_back(Vertex(pos, color, uv, normal));
		}

		// Side triangles
		for (int i = 0; i < slices; ++i)
		{
			uint32 base1 = 1 + i;
			uint32 base2 = 1 + i + 1;
			indices.insert(indices.end(), { apexIndex, base2, base1 });
		}

		// Base cap
		uint32 centerIndex = static_cast<uint32>(vertices.size());
		vertices.push_back(Vertex(ZNVector3(0, 0, 0), color, ZNVector2(0.5f, 0.5f), ZNVector3(0, -1, 0)));

		uint32 baseStart = static_cast<uint32>(vertices.size());
		for (int i = 0; i <= slices; ++i)
		{
			float theta = 2.0f * PI * i / slices;
			float x = radius * cosf(theta);
			float z = radius * sinf(theta);

			ZNVector3 pos(x, 0.0f, z);
			ZNVector2 uv(0.5f + 0.5f * cosf(theta), 0.5f + 0.5f * sinf(theta));
			vertices.push_back(Vertex(pos, color, uv, ZNVector3(0, -1, 0)));
		}

		// Base triangles (winding order reversed for bottom face)
		for (int i = 0; i < slices; ++i)
		{
			uint32 curr = baseStart + i;
			uint32 next = baseStart + i + 1;
			indices.insert(indices.end(), { centerIndex, curr, next });
		}

		ZNMesh* mesh = Platform::CreateMesh();
		mesh->Init(vertices, indices);
		return mesh;
	}

	ZNMesh* ZNMeshFactory::CreatePlane(float size)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32> indices;

		ZNVector4 color(1, 1, 1, 1);
		ZNVector3 normal(0, 1, 0);

		vertices.push_back(Vertex(ZNVector3(-size, 0, -size), color, ZNVector2(0, 0), normal));
		vertices.push_back(Vertex(ZNVector3(size, 0, -size), color, ZNVector2(1, 0), normal));
		vertices.push_back(Vertex(ZNVector3(size, 0, size), color, ZNVector2(1, 1), normal));
		vertices.push_back(Vertex(ZNVector3(-size, 0, size), color, ZNVector2(0, 1), normal));

		indices = { 0, 3, 2, 0, 2, 1 };

		ZNMesh* mesh = Platform::CreateMesh();
		mesh->Init(vertices, indices);
		return mesh;
	}
}
