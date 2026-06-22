#pragma once
#include "ZNSwapChain.h"
#include "ViewMode.h"
#include <functional>

struct ID3D12DescriptorHeap;

namespace ZNFramework
{
	class ZNWindow;
	class ZNSwapChain;
	class ZNCommandQueue
	{
	public:
		ZNCommandQueue() = default;
		virtual ~ZNCommandQueue() noexcept = default;

		virtual void Init(ZNSwapChain* inSwapChain) = 0;
		virtual void RenderBegin() = 0;
		virtual void RenderEnd() = 0;
		virtual void FlushResourceQueue() = 0;
		virtual void WaitSync() = 0;

		virtual float GetGpuFrameTimeMs() const { return 0.0f; }

		void SetForwardRenderCallback(std::function<void()> callback) { forwardRenderCallback = callback; }
		void SetImGuiRenderCallback(std::function<void()> callback) { imguiRenderCallback = callback; }
		void SetImGuiDescriptorHeap(ID3D12DescriptorHeap* heap) { imguiSrvHeap = heap; }

		ViewMode GetViewMode() const { return viewMode; }
		void SetViewMode(ViewMode mode) { viewMode = mode; }

		// Wireframe selection highlight: set once per frame before GBuffer pass.
		// SetWireframeCurrentObject is called per-object in ZNGameObject::Render().
		void SetWireframeSelectedObject(void* obj) { wireframeSelectedObject = obj; }
		void SetWireframeCurrentObject(void* obj)  { wireframeCurrentObject = obj; }
		bool IsCurrentObjectSelected() const {
			return wireframeSelectedObject != nullptr
			    && wireframeCurrentObject == wireframeSelectedObject;
		}

	protected:
		std::function<void()> forwardRenderCallback;
		std::function<void()> imguiRenderCallback;
		ID3D12DescriptorHeap* imguiSrvHeap = nullptr;
		ViewMode viewMode = ViewMode::Lit;
		void* wireframeSelectedObject = nullptr;
		void* wireframeCurrentObject  = nullptr;
	};
}
