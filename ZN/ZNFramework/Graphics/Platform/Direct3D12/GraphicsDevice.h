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

		ComPtr<ID3D12Device> Device() const { return device; }
		ComPtr<IDXGIFactory4> Factory() const { return factory; }

	private:
		ComPtr<ID3D12Debug> debugController;

		ComPtr<ID3D12Device> device;
		ComPtr<IDXGIFactory4> factory;
		ComPtr<ID3D12Fence> fence;
	};
}
