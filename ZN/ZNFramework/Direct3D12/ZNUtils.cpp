#include "ZNUtils.h"

using Microsoft::WRL::ComPtr;

namespace ZNUtils {
	ComPtr<ID3DBlob> CompileShader(
		const std::wstring& fileName,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target) 
	{
		UINT compileFlags{};
#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ComPtr<ID3DBlob> byteCode{ nullptr };
		ComPtr<ID3DBlob> errorBlob{ nullptr };
		HRESULT hr = D3DCompileFromFile(fileName.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errorBlob);

		if (errorBlob)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		ThrowIfFailed(hr);

		return byteCode;
	}

	ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		ComPtr<ID3D12Resource>& uploadBuffer) {

		ComPtr<ID3D12Resource> defaultBuffer{ nullptr };

		D3D12_HEAP_PROPERTIES prop{};
		prop.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC desc{};
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Alignment = 0;
		desc.Width = byteSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

		ThrowIfFailed(device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		ThrowIfFailed(device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

		BYTE* dataBegin{ nullptr };
		HRESULT hr = uploadBuffer.Get()->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin));
		memcpy(dataBegin, initData, byteSize);
		uploadBuffer.Get()->Unmap(0, nullptr);

		cmdList->CopyResource(defaultBuffer.Get(), uploadBuffer.Get());

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Transition.pResource = defaultBuffer.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		cmdList->ResourceBarrier(1, &barrier);

		return defaultBuffer;
	}
}