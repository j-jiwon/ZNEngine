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

	private:
		// window
		D3D12_VIEWPORT viewport = {};
		D3D12_RECT scissorRect = {};
		class ZNTimer* timer;

		// render
		class ZNGraphicsDevice* device = nullptr;
		class ZNCommandQueue* commandQueue = nullptr;
		class ZNSwapChain* swapChain = nullptr;
		class ZNRootSignature* rootSignature = nullptr;
		class ZNTableDescriptorHeap* tableDescriptorHeap = nullptr;

		class ZNShader* defaultShader = nullptr;
		class ZNMesh* defaultMesh = nullptr;
		class ZNTexture* defaultTexture = nullptr;
		class ZNMaterial* defaultMaterial = nullptr;
		class ZNConstantBuffer* constantBuffer = nullptr;
		class ZNDepthStencilBuffer* depthStencilBuffer = nullptr;
		class ZNCamera* camera = nullptr;

		// Lights
		class ZNSpotLight* spotLight = nullptr;
		class ZNDirectionalLight* directionalLight = nullptr;

		// FBX Model test
		std::vector<class ZNMesh*> loadedMeshes;
		std::vector<class ZNMaterial*> loadedMaterials;
		std::vector<class ZNTexture*> loadedTextures;

		// Debug visualization
		class ZNMesh* crosshairMesh = nullptr;
		class ZNMesh* lightDebugMesh = nullptr;
		class ZNMesh* axisXMesh = nullptr;
		class ZNMesh* axisYMesh = nullptr;
		class ZNMesh* axisZMesh = nullptr;
		class ZNMaterial* debugMaterial = nullptr;
		class ZNMaterial* redMaterial = nullptr;
		class ZNMaterial* greenMaterial = nullptr;
		class ZNMaterial* blueMaterial = nullptr;
	};
}

#endif