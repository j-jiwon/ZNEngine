#include "ZNGameObject.h"
#include "../ZNFramework.h"
#include "../Graphics/ZNMesh.h"
#include "../Graphics/ZNMaterial.h"

using namespace ZNFramework;

void ZNGameObject::Render()
{
	if (!mesh)
		return;

	// Set transform
	mesh->SetTransform(transform);

	// Render mesh (material is already set on mesh if available)
	mesh->Render();
}
