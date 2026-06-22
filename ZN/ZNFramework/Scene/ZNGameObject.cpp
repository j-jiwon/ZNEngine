#include "ZNGameObject.h"
#include "../ZNFramework.h"
#include "../Graphics/ZNMesh.h"
#include "../Graphics/ZNMaterial.h"
#include "../Math/ZNMatrix4.h"

using namespace ZNFramework;

int ZNGameObject::sDrawCalls = 0;
int ZNGameObject::sLastFrameDrawCalls = 0;
int ZNGameObject::sTriangles = 0;
int ZNGameObject::sLastFrameTriangles = 0;
int ZNGameObject::sVertices = 0;
int ZNGameObject::sLastFrameVertices = 0;

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
		sTriangles += static_cast<int>(mesh->GetIndexCount() / 3);
		sVertices  += static_cast<int>(mesh->GetVertexCount());

		ZNCommandQueue* cq = GraphicsContext::GetInstance().GetCommandQueue();
		cq->SetWireframeCurrentObject(this);
		mesh->Render();
		cq->SetWireframeCurrentObject(nullptr);
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
		sTriangles += static_cast<int>(mesh->GetIndexCount() / 3);
		sVertices  += static_cast<int>(mesh->GetVertexCount());
		mesh->RenderShadow(lightViewProj, shadowShader);
	}
}
