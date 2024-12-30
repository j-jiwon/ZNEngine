#pragma once
#include "ZNFramework.h"

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
		ZNConstantBuffer* CreateConstantBuffer();
		ZNTableDescriptorHeap* CreateTableDescriptorHeap();
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
	inline ZNConstantBuffer* CreateConstantBuffer()
	{
		return Direct3D::CreateConstantBuffer();
	}
	inline ZNTableDescriptorHeap* CreateTableDescriptorHeap()
	{
		return Direct3D::CreateTableDescriptorHeap();
	}
}
