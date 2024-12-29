#pragma once
#include "../ZNGraphicsDevice.h"
#include "../ZNCommandQueue.h"
#include "../ZNSwapChain.h"
#include "../ZNRootSignature.h"
#include "Direct3D12/GraphicsDevice.h"
#include "../ZNGraphicsContext.h"
#include "../ZNShader.h"
#include "../ZNMesh.h"

namespace ZNFramework::Platform
{
#if defined(_WIN32)
	namespace Direct3D
	{
		ZNGraphicsDevice* CreateGraphicsDevice();
		ZNCommandQueue* CreateCommandQueue();
		ZNSwapChain* CreateSwapChain();
		ZNRootSignature* CreateRootSignature();
		ZNShader* CreateShader();
		ZNMesh* CreateMesh();
	}
#endif

	inline ZNGraphicsDevice* CreateGraphicsDevice()
	{
		return Direct3D::CreateGraphicsDevice();
	}
	inline ZNCommandQueue* CreateCommandQueue()
	{
		return Direct3D::CreateCommandQueue();
	}
	inline ZNSwapChain* CreateSwapChain()
	{
		return Direct3D::CreateSwapChain();
	}
	inline ZNRootSignature* CreateRootSignature()
	{
		return Direct3D::CreateRootSignature();
	}
	inline ZNShader* CreateShader()
	{
		return Direct3D::CreateShader();
	}
	inline ZNMesh* CreateMesh()
	{
		return Direct3D::CreateMesh();
	}
}
