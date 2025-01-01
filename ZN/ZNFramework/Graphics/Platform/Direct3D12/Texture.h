#pragma once
#include "Graphics/ZNTexture.h"
#include "ZNUtils.h"

namespace ZNFramework
{
	class Texture : public ZNTexture
	{
	public:
		void Init(const std::wstring& path) override;
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() { return srvHandle; }

	private:
		void CreateTexture(const std::wstring& path);
		void CreateView();

	private:
		DirectX::ScratchImage image;
		ComPtr<ID3D12Resource> tex2d;
		ComPtr<ID3D12DescriptorHeap> srvHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
	};
}
