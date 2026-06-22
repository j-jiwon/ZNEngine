#pragma once
#ifdef _WIN32
#include "UI/ZNImGuiLayer.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/ZNUtils.h"
#include <Windows.h>
#include "imgui.h"

namespace ZNFramework::Platform::Direct3D
{
	class ImGuiLayer : public ZNImGuiLayer
	{
	public:
		ImGuiLayer() = default;
		~ImGuiLayer() noexcept = default;

		void Init(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue, int bufferCount, DXGI_FORMAT rtvFormat);
		void Shutdown() override;
		void BeginFrame() override;
		void EndFrame() override;

		ID3D12DescriptorHeap* GetSrvHeap() const { return srvHeap.Get(); }
		ImTextureID AllocateTexture(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srcSrv);
		ImTextureID AllocateGrayscaleTexture(ID3D12Device* device, ID3D12Resource* resource, DXGI_FORMAT format);
		ImTextureID SetTexture(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srcSrv, int slot);
		ImTextureID SetGrayscaleTexture(ID3D12Device* device, ID3D12Resource* resource, DXGI_FORMAT format, int slot);

	private:
		ComPtr<ID3D12DescriptorHeap> srvHeap;
		int nextTextureSlot = 1; // slot 0 is reserved for font texture
	};
}
#endif
