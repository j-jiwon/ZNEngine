#include "ZNGameObject.h"
#include "../ZNFramework.h"
#include "../Graphics/ZNMesh.h"
#include "../Graphics/ZNMaterial.h"
#include "../Math/ZNMatrix4.h"

using namespace ZNFramework;

int ZNGameObject::sDrawCalls = 0;
int ZNGameObject::sLastFrameDrawCalls = 0;

void ZNGameObject::Render()
{
	if (!mesh)
		return;

	// Set transform
	mesh->SetTransform(transform);

	// Render mesh (material is already set on mesh if available)
	if (isVisible)
	{
		++sDrawCalls;
		mesh->Render();
	}
}

void ZNGameObject::RenderShadow(const ZNMatrix4& lightViewProj, ZNShader* shadowShader)
{
	if (!mesh || !castShadow) return;

	// Set transform
	mesh->SetTransform(transform);

	// Render mesh for shadow pass
	if (isVisible)
	{
		++sDrawCalls;
		mesh->RenderShadow(lightViewProj, shadowShader);
	}
}
