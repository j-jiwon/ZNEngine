#pragma once

namespace ZNFramework
{
	class ZNMesh;

	class ZNMeshFactory
	{
	public:
		// Creates a cube mesh with the given size (half-extent)
		// size = 1.0 creates a 2x2x2 cube centered at origin
		static ZNMesh* CreateCube(float size = 1.0f);

		// Creates a UV sphere mesh
		// radius: sphere radius
		// stacks: number of horizontal divisions (latitude)
		// slices: number of vertical divisions (longitude)
		static ZNMesh* CreateSphere(float radius = 1.0f, int stacks = 16, int slices = 16);

		// Creates a cone mesh
		// radius: base radius
		// height: cone height
		// slices: number of divisions around the base
		static ZNMesh* CreateCone(float radius = 1.0f, float height = 2.0f, int slices = 16);

		// Creates a plane mesh (XZ plane)
		// size: half-extent of the plane
		static ZNMesh* CreatePlane(float size = 1.0f);

	private:
		ZNMeshFactory() = delete;
	};
}
