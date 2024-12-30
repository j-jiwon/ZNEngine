#pragma once
#ifdef _WIN32
#include "../../ZNApplicationContextInterface.h"
#include "../../../../ZNFramework.h"


namespace ZNFramework
{
	class ApplicationContext : public ZNApplicationContextInterface
	{
	public:
		ApplicationContext() = default;
		~ApplicationContext() = default;
		int MessageLoop() override;

		void Initialize(ZNWindow* inWindow, ZNGraphicsDevice* inDevice) override;

		void OnResize(size_t width, size_t height);
		void Update();
		
		void Render();
		void RenderBegin();
		void RenderEnd();

	private:
		// window
		D3D12_VIEWPORT viewport = {};
		D3D12_RECT scissorRect = {};

		// render
		class ZNGraphicsDevice* device = nullptr;
		class ZNCommandQueue* commandQueue = nullptr;
		class ZNSwapChain* swapChain = nullptr;
		class ZNRootSignature* rootSignature = nullptr;
		class ZNTableDescriptorHeap* tableDescriptorHeap = nullptr;

		class ZNShader* defaultShader = nullptr;
		class ZNMesh* defaultMesh = nullptr;
		class ZNConstantBuffer* constantBuffer = nullptr;
	};
}

#endif