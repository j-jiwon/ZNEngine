#pragma once
#ifdef _WIN32
#include "UI/ZNImGuiLayer.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/ZNUtils.h"
#include <Windows.h>

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

	private:
		ComPtr<ID3D12DescriptorHeap> srvHeap;
	};
}
#endif
