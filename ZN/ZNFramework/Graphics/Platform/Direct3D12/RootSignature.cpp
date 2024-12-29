#include "RootSignature.h"
#include "GraphicsDevice.h"
#include "../../ZNGraphicsContext.h"
#include "../../../../ZNFramework.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNRootSignature* CreateRootSignature()
	{
		return new RootSignature();
	}
}

void RootSignature::Init()
{
	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

	D3D12_ROOT_SIGNATURE_DESC signatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(D3D12_DEFAULT);
	signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> blobSignature;
	ComPtr<ID3DBlob> blobError;
	::D3D12SerializeRootSignature(&signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature, &blobError);
	device->Device()->CreateRootSignature(0, blobSignature->GetBufferPointer(), blobSignature->GetBufferSize(), IID_PPV_ARGS(&signature));
}
