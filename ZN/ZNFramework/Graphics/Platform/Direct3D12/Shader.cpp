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

	// Store input layout descriptors as member variable to keep them valid
	inputElementDescs = {
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 12,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // 12 = float3 pos
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,		0, 28,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // 28 = 12 + 16(float4 color)
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 36,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }  // 36 = 12 + 16 + 8(float2 uv)
	};

	pipelineDesc.InputLayout = { inputElementDescs.data(), static_cast<UINT>(inputElementDescs.size()) };
	RootSignature* rootSignature = GraphicsContext::GetInstance().GetAs<RootSignature>();
	pipelineDesc.pRootSignature = rootSignature->GetSignature().Get();

	pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
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
	ThrowIfFailed(device->Device()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState)));
	CreateWireframePSO();
}

void Shader::Bind()
{
	CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	ID3D12PipelineState* pso =
		(queue->GetViewMode() == ViewMode::Wireframe && pipelineStateWireframe)
		? pipelineStateWireframe.Get() : pipelineState.Get();
	queue->CommandList()->SetPipelineState(pso);
}

void Shader::CreateWireframePSO()
{
	// Depth-only pipelines (shadow maps) have 0 render targets and must not respond
	// to wireframe mode — shadow geometry must remain filled to produce correct shadows.
	if (pipelineDesc.NumRenderTargets == 0)
	{
		pipelineStateWireframe = nullptr;
		return;
	}

	auto wireDesc = pipelineDesc;
	wireDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	wireDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	pipelineStateWireframe.Reset();
	ThrowIfFailed(device->Device()->CreateGraphicsPipelineState(&wireDesc, IID_PPV_ARGS(&pipelineStateWireframe)));
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
		if (errBlob)
		{
			string errorMsg = "Shader Compile Failed!\n\n";
			errorMsg += static_cast<const char*>(errBlob->GetBufferPointer());
			::MessageBoxA(nullptr, errorMsg.c_str(), "Shader Error", MB_OK);
		}
		else
		{
			::MessageBoxA(nullptr, "Shader Create Failed !", nullptr, MB_OK);
		}
		return;
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

void Shader::SetRenderTargetFormats(uint32 numRenderTargets, const DXGI_FORMAT* formats)
{
	pipelineDesc.NumRenderTargets = numRenderTargets;

	// Clear all RTV formats first
	for (uint32 i = 0; i < 8; ++i)
	{
		pipelineDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}

	// Set provided formats
	for (uint32 i = 0; i < numRenderTargets; ++i)
	{
		pipelineDesc.RTVFormats[i] = formats[i];
	}

	// For depth-only pass (no render targets), configure for shadow mapping
	if (numRenderTargets == 0)
	{
		// Set depth format for shadow map
		pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		// Disable pixel shader for depth-only rendering (more efficient)
		pipelineDesc.PS = { nullptr, 0 };
	}

	// Recreate pipeline state with new configuration
	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	pipelineState.Reset();
	ThrowIfFailed(device->Device()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState)));
	CreateWireframePSO();
}

void Shader::DisableDepthTest()
{
	pipelineDesc.DepthStencilState.DepthEnable = FALSE;
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	pipelineDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	pipelineState.Reset();
	ThrowIfFailed(device->Device()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState)));
	CreateWireframePSO();
}

void Shader::DisableDepthWrite()
{
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	pipelineState.Reset();
	ThrowIfFailed(device->Device()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState)));
	CreateWireframePSO();
}

void Shader::EnableAlphaBlend()
{
	D3D12_RENDER_TARGET_BLEND_DESC& rtBlend = pipelineDesc.BlendState.RenderTarget[0];
	rtBlend.BlendEnable = TRUE;
	rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	pipelineState.Reset();
	ThrowIfFailed(device->Device()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState)));
	CreateWireframePSO();
}
