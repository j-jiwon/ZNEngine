#pragma once


namespace ZNFramework
{
	class ZNWindow;
	class ZNGraphicsDevice;
	class ZNScene;
	class ZNApplicationContextInterface
	{
	public:
		ZNApplicationContextInterface() = default;
		~ZNApplicationContextInterface() = default;

		virtual void Initialize(ZNWindow* window, ZNGraphicsDevice* device) = 0;
		virtual int MessageLoop() = 0;
		virtual void SetScene(ZNScene* scene) = 0;
		virtual ZNScene* GetScene() const = 0;
	};
}