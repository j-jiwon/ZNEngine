#pragma once
#include "Graphics/ZNShader.h"
#include "ZNUtils.h"

using namespace std;

namespace ZNFramework
{
	class Shader : public ZNShader
	{
	public:
		void Load(const wstring& path) override;
		void Bind() override;

		void SetRenderTargetFormats(uint32 numRenderTargets, const DXGI_FORMAT* formats);
		void DisableDepthTest() override;
		void DisableDepthWrite() override;
		void EnableAlphaBlend() override;

	private:
		void CreateShader(const wstring& path, const string& name, const string& version, ComPtr<ID3DBlob>& blob, D3D12_SHADER_BYTECODE& shaderByteCode);
		void CreateVertexShader(const wstring& path, const string& name, const string& version);
		void CreatePixelShader(const wstring& path, const string& name, const string& version);
		void CreateWireframePSO();

	private:
		ComPtr<ID3DBlob> vsBlob;
		ComPtr<ID3DBlob> psBlob;
		ComPtr<ID3DBlob> errBlob;

		ComPtr<ID3D12PipelineState>	pipelineState;
		ComPtr<ID3D12PipelineState>	pipelineStateWireframe;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};

		// Store input layout descriptors to keep them valid
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
	};
}
