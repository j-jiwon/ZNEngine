#pragma once
#ifdef _WIN32
#include "Application/ZNApplicationContextInterface.h"
#include "ZNFramework.h"


namespace ZNFramework
{
	class ApplicationContext : public ZNApplicationContextInterface
	{
	public:
		ApplicationContext() = default;
		~ApplicationContext() = default;
		int MessageLoop() override;

		void Initialize(ZNWindow* inWindow, ZNGraphicsDevice* inDevice) override;

		void OnResize(uint32 width, uint32 height);
		void OnMouseEvent(struct MouseEvent event);
		void OnKeyboardEvent(struct KeyboardEvent event);
		void Update();

		void Render();
		void RenderBegin();
		void RenderEnd();

		// Scene management
		void SetScene(ZNScene* scene) override;
		ZNScene* GetScene() const override;

	private:
		// window
		class ZNTimer* timer;

		// render
		class ZNGraphicsDevice* device = nullptr;
		class ZNCommandQueue* commandQueue = nullptr;
		class ZNSwapChain* swapChain = nullptr;
		class ZNRootSignature* rootSignature = nullptr;
		class ZNTableDescriptorHeap* tableDescriptorHeap = nullptr;
		class ZNShader* defaultShader = nullptr;
		class ZNShader* gbufferShader = nullptr;
		class ZNShader* shadowDepthShader = nullptr;
		class ZNConstantBuffer* constantBuffer = nullptr;
		class ZNDepthStencilBuffer* depthStencilBuffer = nullptr;

		// Current scene
		class ZNScene* currentScene = nullptr;
	};
}

#endif