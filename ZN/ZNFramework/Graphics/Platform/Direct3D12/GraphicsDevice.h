#pragma once
#include "../../ZNGraphicsDevice.h"
#include "ZNUtils.h"

namespace ZNFramework
{
	class GraphicsDevice : public ZNGraphicsDevice
	{
	public:
		GraphicsDevice();
		~GraphicsDevice() noexcept = default;

		ZNCommandQueue* CreateCommandQueue() override;
		ZNCommandList* CreateCommandList() override;

		ID3D12Device* Device() const { return device.Get(); }
		ComPtr<IDXGIFactory4> Factory() const { return factory; }

	private:
		ComPtr<ID3D12Device> device;
		ComPtr<IDXGIFactory4> factory;
	};
}
