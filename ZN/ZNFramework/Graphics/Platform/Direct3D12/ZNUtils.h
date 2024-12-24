#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include <system_error>
#include <assert.h>
#include <D3Dcompiler.h>
//#include <DirectXMath.h>
//#include <DirectXCollision.h>
//#include <unordered_map>

using namespace Microsoft::WRL;

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        auto msg = std::system_category().message(hr);
		printf("RESULT: 0x%08X\n", hr);
        throw std::exception(msg.c_str());
    }
}

enum
{
	SWAP_CHAIN_BUFFER_COUNT = 2
};

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

//namespace VectorMath {
//	constexpr DirectX::XMFLOAT4X4 Identity4X4() {
//		DirectX::XMFLOAT4X4 i{
//			1.0f, 0.0f, 0.0f, 0.0f,
//			0.0f, 1.0f, 0.0f, 0.0f,
//			0.0f, 0.0f, 1.0f, 0.0f,
//			0.0f, 0.0f, 0.0f, 1.0f };
//
//		return i;
//	}
//} 
/*
namespace VectorMath {
	constexpr DirectX::XMFLOAT4X4 Identity4X4() {
		DirectX::XMFLOAT4X4 i{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f };

		return i;
	}
} 

namespace ZNUtils {
    constexpr UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        return (byteSize + 255) & ~255;
    }
	template <typename T>
	
	class UploadBuffer {
	public:

		UploadBuffer(ID3D12Device* device, std::uint32_t elementCount, bool isConstantBuffer)
			: mIsConstantBuffer{ isConstantBuffer } {
			if (isConstantBuffer)
				byteSize = ZNUtils::CalcConstantBufferByteSize(sizeof(T));

			D3D12_HEAP_PROPERTIES prop{};
			prop.Type = D3D12_HEAP_TYPE_UPLOAD;

			D3D12_RESOURCE_DESC desc{};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Width = elementCount * byteSize;
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;

			ThrowIfFailed(device->CreateCommittedResource(
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadBuffer)));

			ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

		}
		~UploadBuffer() {
			if (uploadBuffer)
				uploadBuffer->Unmap(0, nullptr);

			uploadBuffer = nullptr;
		}

		ID3D12Resource* GetResource() const {
			return uploadBuffer.Get();
		}

		void CopyData(int elementIndex, const T& data) {
			memcpy(&mappedData[elementIndex * byteSize], &data, sizeof(T));
		}

		UploadBuffer(const UploadBuffer& rhs) = delete;
		UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

	private:
		bool mIsConstantBuffer{ false };
		std::uint32_t byteSize{};

		Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer{};
		BYTE* mappedData{ nullptr };
	};

	Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);
}
*/