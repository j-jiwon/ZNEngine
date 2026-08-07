#include "Shader.h"
#include "GraphicsDevice.h"
#include "RootSignature.h"
#include "CommandQueue.h"
#include "DepthStencilBuffer.h"
#include "ZNFramework.h"
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

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
	ComPtr<ID3DBlob> newVS = CompileShader(path, "VS_Main", "vs_5_0");
	ComPtr<ID3DBlob> newPS = CompileShader(path, "PS_Main", "ps_5_0");

	vsBlob = std::move(newVS);
	psBlob = std::move(newPS);
	sourcePath = std::filesystem::path(path).string();
	pipelineDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	pipelineDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

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

	// PSO is built lazily — on first Bind(), or when a state setter reconfigures pipelineDesc.
	// Deferring avoids a throwaway default 1-RTV PSO for shaders whose real formats are set after
	// Load (e.g. the 5-target G-buffer shader, whose 5 PS outputs would mismatch a 1-RTV PSO and
	// trip the debug layer at creation).
}

void Shader::Bind()
{
	if (!pipelineState) // built lazily (see Load) — this shader was only Load()'ed, no state setter
		RebuildPSO();

	CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	ID3D12PipelineState* pso =
		(queue->GetViewMode() == ViewMode::Wireframe && pipelineStateWireframe)
		? pipelineStateWireframe.Get() : pipelineState.Get();
	queue->CommandList()->SetPipelineState(pso);
}

ComPtr<ID3D12PipelineState> Shader::CreateWireframePSO()
{
	// Depth-only pipelines (shadow maps) have 0 render targets and must not respond
	// to wireframe mode — shadow geometry must remain filled to produce correct shadows.
	if (pipelineDesc.NumRenderTargets == 0)
	{
		return {};
	}

	auto wireDesc = pipelineDesc;
	wireDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	wireDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	ComPtr<ID3D12PipelineState> wireframeState;
	const HRESULT hr = device->Device()->CreateGraphicsPipelineState(&wireDesc, IID_PPV_ARGS(&wireframeState));
	if (FAILED(hr))
	{
		std::ostringstream message;
		message << "shader wireframe PSO creation failed: path=" << sourcePath
			<< ", HRESULT=0x" << std::hex << std::uppercase << static_cast<unsigned long>(hr);
		throw std::runtime_error(message.str());
	}
	return wireframeState;
}

void Shader::RebuildPSO()
{
	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	ComPtr<ID3D12PipelineState> newPipelineState;
	const HRESULT hr = device->Device()->CreateGraphicsPipelineState(
		&pipelineDesc, IID_PPV_ARGS(&newPipelineState));
	if (FAILED(hr))
	{
		std::ostringstream message;
		message << "shader PSO creation failed: path=" << sourcePath
			<< ", HRESULT=0x" << std::hex << std::uppercase << static_cast<unsigned long>(hr);
		throw std::runtime_error(message.str());
	}
	ComPtr<ID3D12PipelineState> newWireframeState = CreateWireframePSO();

	pipelineState = std::move(newPipelineState);
	pipelineStateWireframe = std::move(newWireframeState);
}

ComPtr<ID3DBlob> Shader::CompileShader(const wstring& path, const string& name, const string& version)
{
	uint32 compileFlag = 0;
#ifdef _DEBUG
	compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> errorBlob;
	const HRESULT hr = ::D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		name.c_str(), version.c_str(), compileFlag, 0, &blob, &errorBlob);
	if (FAILED(hr))
	{
		std::ostringstream message;
		message << "shader compile failed: path=" << std::filesystem::path(path).string()
			<< ", entry=" << name << ", target=" << version
			<< ", HRESULT=0x" << std::hex << std::uppercase << static_cast<unsigned long>(hr);
		if (errorBlob && errorBlob->GetBufferPointer())
			message << "\n" << static_cast<const char*>(errorBlob->GetBufferPointer());
		throw std::runtime_error(message.str());
	}

	return blob;
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
	RebuildPSO();
}

void Shader::DisableDepthTest()
{
	pipelineDesc.DepthStencilState.DepthEnable = FALSE;
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	pipelineDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	RebuildPSO();
}

void Shader::DisableDepthWrite()
{
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	RebuildPSO();
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

	RebuildPSO();
}

void Shader::EnableAdditiveBlend()
{
	D3D12_RENDER_TARGET_BLEND_DESC& rtBlend = pipelineDesc.BlendState.RenderTarget[0];
	rtBlend.BlendEnable = TRUE;
	rtBlend.SrcBlend = D3D12_BLEND_ONE;
	rtBlend.DestBlend = D3D12_BLEND_ONE;
	rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlend.DestBlendAlpha = D3D12_BLEND_ONE;
	rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	RebuildPSO();
}
