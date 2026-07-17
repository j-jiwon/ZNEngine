#include "ZNGameObject.h"
#include <algorithm>
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

void ZNGameObject::AddChild(ZNGameObject* child)
{
	if (!child || child == this || child->parent == this)
		return;
	child->DetachFromParent();
	child->parent = this;
	children.push_back(child);
}

void ZNGameObject::DetachFromParent()
{
	if (!parent)
		return;
	auto& siblings = parent->children;
	siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
	parent = nullptr;
}

ZNMatrix4 ZNGameObject::GetLocalMatrix() const
{
	return transform.GetWorldMatrix();
}

ZNMatrix4 ZNGameObject::GetWorldMatrix() const
{
	// Row-vector convention (v * S*R*T), matching Transform::GetWorldMatrix():
	// apply this node's local transform first, then the parent's world transform.
	return parent ? (GetLocalMatrix() * parent->GetWorldMatrix()) : GetLocalMatrix();
}

void ZNGameObject::Render()
{
	if (!mesh)
		return;

	// Push composed world matrix (walks the parent chain)
	mesh->SetWorldMatrix(GetWorldMatrix());

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

	// Push composed world matrix (walks the parent chain)
	mesh->SetWorldMatrix(GetWorldMatrix());

	// Render mesh for shadow pass
	if (isVisible)
	{
		++sDrawCalls;
		sTriangles += static_cast<int>(mesh->GetIndexCount() / 3);
		sVertices  += static_cast<int>(mesh->GetVertexCount());
		mesh->RenderShadow(lightViewProj, shadowShader);
	}
}
