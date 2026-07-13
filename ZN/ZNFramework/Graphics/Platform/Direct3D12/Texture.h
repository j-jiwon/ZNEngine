#pragma once
#include "Graphics/ZNTexture.h"
#include "ZNUtils.h"

namespace ZNFramework
{
	class Texture : public ZNTexture
	{
	public:
		void Init(const std::wstring& path) override;
		void InitFromMemory(const void* data, size_t size) override;
		void InitSolidColor(uint8 r, uint8 g, uint8 b, uint8 a) override;
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() { return srvHandle; }

	private:
		void CreateTexture(const std::wstring& path);
		void UploadToGPU();
		void CreateView();

	private:
		DirectX::ScratchImage image;
		ComPtr<ID3D12Resource> tex2d;
		ComPtr<ID3D12DescriptorHeap> srvHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
	};
}
