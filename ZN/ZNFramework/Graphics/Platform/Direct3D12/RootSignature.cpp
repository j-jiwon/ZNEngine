#include "RootSignature.h"
#include "GraphicsDevice.h"
#include "ZNFramework.h"

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
	CreateSamplerDesc();
	CreateRootSignature();
}

void RootSignature::CreateRootSignature()
{
	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

	CD3DX12_DESCRIPTOR_RANGE ranges[] =
	{
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CBV_REGISTER_COUNT, 0), // b0~b4
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRV_REGISTER_COUNT + 1, 0), // t0~t5 (added shadow map)
	};

	CD3DX12_ROOT_PARAMETER param[1];
	param[0].InitAsDescriptorTable(_countof(ranges), ranges);
	D3D12_ROOT_SIGNATURE_DESC signatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(param), param, 2, samplerDesc);
	signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> blobSignature;
	ComPtr<ID3DBlob> blobError;
	::D3D12SerializeRootSignature(&signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature, &blobError);
	device->Device()->CreateRootSignature(0, blobSignature->GetBufferPointer(), blobSignature->GetBufferSize(), IID_PPV_ARGS(&signature));
}

void RootSignature::CreateSamplerDesc()
{
	// s0: Default sampler for texture sampling
	samplerDesc[0] = CD3DX12_STATIC_SAMPLER_DESC(0);

	// s1: Comparison sampler for shadow mapping (PCF)
	samplerDesc[1] = CD3DX12_STATIC_SAMPLER_DESC(
		1,  // shaderRegister (s1)
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		0.0f,  // mipLODBias
		16,    // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE  // Outside shadow map = fully lit
	);
}
