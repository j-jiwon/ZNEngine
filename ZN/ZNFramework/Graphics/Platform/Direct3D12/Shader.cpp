#include "Shader.h"
#include "GraphicsDevice.h"
#include "RootSignature.h"
#include "CommandQueue.h"
#include "DepthStencilBuffer.h"
#include "ZNFramework.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNShader* CreateShader()
	{
		return new Shader();
	}
}

void Shader::Load(const wstring& path)
{
	CreateVertexShader(path, "VS_Main", "vs_5_0");
	CreatePixelShader(path, "PS_Main", "ps_5_0");

	D3D12_INPUT_ELEMENT_DESC desc[] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 12,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // 12 = float3 pos
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,		0, 28,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }  // 28 = 12 + 16(float4 color)
	};

	pipelineDesc.InputLayout = { desc, _countof(desc) };
	RootSignature* rootSignature = GraphicsContext::GetInstance().GetAs<RootSignature>();
	pipelineDesc.pRootSignature = rootSignature->GetSignature().Get();

	pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pipelineDesc.SampleMask = UINT_MAX;
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.NumRenderTargets = 1;
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineDesc.SampleDesc.Count = 1;
	DepthStencilBuffer* dsBuffer = GraphicsContext::GetInstance().GetAs<DepthStencilBuffer>();
	pipelineDesc.DSVFormat = dsBuffer->GetDSVFormat();

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	device->Device()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState));
}

void Shader::Bind()
{
	CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	queue->CommandList()->SetPipelineState(pipelineState.Get());
}

void Shader::CreateShader(const wstring& path, const string& name, const string& version, ComPtr<ID3DBlob>& blob, D3D12_SHADER_BYTECODE& shaderByteCode)
{
	uint32 compileFlag = 0;
#ifdef _DEBUG
	compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	if (FAILED(::D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE
		, name.c_str(), version.c_str(), compileFlag, 0, &blob, &errBlob)))
	{
		::MessageBoxA(nullptr, "Shader Create Failed !", nullptr, MB_OK);
	}

	shaderByteCode = { blob->GetBufferPointer(), blob->GetBufferSize() };
}

void Shader::CreateVertexShader(const wstring& path, const string& name, const string& version)
{
	CreateShader(path, name, version, vsBlob, pipelineDesc.VS);
}

void Shader::CreatePixelShader(const wstring& path, const string& name, const string& version)
{
	CreateShader(path, name, version, psBlob, pipelineDesc.PS);
}
