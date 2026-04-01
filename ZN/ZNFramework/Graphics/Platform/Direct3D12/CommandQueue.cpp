#include "CommandQueue.h"
#include "SwapChain.h"
#include "RootSignature.h"
#include "ConstantBuffer.h"
#include "GraphicsDevice.h"
#include "TableDescriptorHeap.h"
#include "DepthStencilBuffer.h"
#include "GBufferManager.h"
#include "DebugViewportRenderer.h"
#include "ZNFramework.h"

using namespace ZNFramework;

namespace ZNFramework::Platform::Direct3D
{
	ZNCommandQueue* CreateCommandQueue()
	{
		return new CommandQueue();
	}
}

void CommandQueue::Init(ZNSwapChain* inSwapChain)
{
	// init variables
	device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	swapChain = dynamic_cast<SwapChain*>(inSwapChain);

	// init
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	device->Device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
	device->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	device->Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	commandList->Close();

	device->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&resourceCommandAllocator));
	device->Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, resourceCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&resourceCommandList));

	device->Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void CommandQueue::RenderBegin()
{
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	RootSignature* rootSignature = GraphicsContext::GetInstance().GetAs<RootSignature>();
	commandList->SetGraphicsRootSignature(rootSignature->GetSignature().Get());
	ConstantBuffer* constantBuffer = GraphicsContext::GetInstance().GetAs<ConstantBuffer>();
	constantBuffer->Clear();
	TableDescriptorHeap* tableDescHeap = GraphicsContext::GetInstance().GetAs<TableDescriptorHeap>();
	tableDescHeap->Clear();
	ID3D12DescriptorHeap* descHeap = tableDescHeap->GetDescriptorHeap().Get();
	commandList->SetDescriptorHeaps(1, &descHeap);

	D3D12_VIEWPORT vp = { 0, 0, static_cast<FLOAT>(swapChain->Width()), static_cast<FLOAT>(swapChain->Height()), 0.f, 1.f };
	D3D12_RECT rect = CD3DX12_RECT(0, 0, swapChain->Width(), swapChain->Height());

	commandList->RSSetViewports(1, &vp);
	commandList->RSSetScissorRects(1, &rect);

	if (enableGBuffer)
	{
		// Bind G-Buffer shader for MRT rendering
		ZNShader* gbufferShader = GraphicsContext::GetInstance().GetGBufferShader();
		if (gbufferShader)
		{
			gbufferShader->Bind();
		}

		// Transition G-Buffer resources from SHADER_RESOURCE to RENDER_TARGET
		// Skip on first frame since resources are created in RENDER_TARGET state
		if (!isFirstFrame)
		{
			D3D12_RESOURCE_BARRIER barriers[3];
			barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
				gbufferManager->GetBaseColorResource(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
				gbufferManager->GetNormalResource(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(
				gbufferManager->GetDepthCopyResource(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);

			commandList->ResourceBarrier(3, barriers);
		}

		// Clear all G-Buffer render targets
		float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		commandList->ClearRenderTargetView(gbufferManager->GetBaseColorRTV(), clearColor, 0, nullptr);

		float clearNormal[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		commandList->ClearRenderTargetView(gbufferManager->GetNormalRTV(), clearNormal, 0, nullptr);

		float clearDepth[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
		commandList->ClearRenderTargetView(gbufferManager->GetDepthCopyRTV(), clearDepth, 0, nullptr);

		// Set G-Buffer render targets
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[3];
		rtvHandles[0] = gbufferManager->GetBaseColorRTV();
		rtvHandles[1] = gbufferManager->GetNormalRTV();
		rtvHandles[2] = gbufferManager->GetDepthCopyRTV();

		// depth stencil
		DepthStencilBuffer* dsBuffer = GraphicsContext::GetInstance().GetAs<DepthStencilBuffer>();
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = dsBuffer->GetDSVCpuHandle();
		commandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// Bind G-Buffer render targets with depth stencil
		commandList->OMSetRenderTargets(3, rtvHandles, FALSE, &depthStencilView);
	}
	else
	{
		// Original single render target path
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			swapChain->GetBackRTVBuffer().Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList->ResourceBarrier(1, &barrier);

		D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = swapChain->GetBackRTV();
		float clearColor[4] = { 0.2f, 0.3f, 0.4f, 1.0f };
		commandList->ClearRenderTargetView(backBufferView, clearColor, 0, nullptr);

		// depth stencil
		DepthStencilBuffer* dsBuffer = GraphicsContext::GetInstance().GetAs<DepthStencilBuffer>();
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = dsBuffer->GetDSVCpuHandle();
		commandList->OMSetRenderTargets(1, &backBufferView, FALSE, &depthStencilView);
		commandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}

void CommandQueue::RenderEnd()
{
	if (enableGBuffer)
	{
		// Transition G-Buffer resources back to SHADER_RESOURCE for reading in debug views
		D3D12_RESOURCE_BARRIER barriers[3];
		barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
			gbufferManager->GetBaseColorResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
			gbufferManager->GetNormalResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(
			gbufferManager->GetDepthCopyResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandList->ResourceBarrier(3, barriers);

		// Transition back buffer to RENDER_TARGET for composite/debug rendering
		D3D12_RESOURCE_BARRIER backBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			swapChain->GetBackRTVBuffer().Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &backBufferBarrier);

		// Clear back buffer
		D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = swapChain->GetBackRTV();
		float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black background
		commandList->ClearRenderTargetView(backBufferView, clearColor, 0, nullptr);

		// Set back buffer as render target
		commandList->OMSetRenderTargets(1, &backBufferView, FALSE, nullptr);

		// Restore full viewport
		D3D12_VIEWPORT vp = { 0, 0, static_cast<FLOAT>(swapChain->Width()), static_cast<FLOAT>(swapChain->Height()), 0.f, 1.f };
		D3D12_RECT rect = CD3DX12_RECT(0, 0, swapChain->Width(), swapChain->Height());
		commandList->RSSetViewports(1, &vp);
		commandList->RSSetScissorRects(1, &rect);

		// Render G-Buffer BaseColor to main view (fullscreen)
		if (debugViewportRenderer)
		{
			debugViewportRenderer->RenderMainView(gbufferManager->GetBaseColorSRV(), swapChain->Width(), swapChain->Height());
		}

		// Render debug viewports on top
		if (debugViewportRenderer)
		{
			debugViewportRenderer->RenderDebugViews(gbufferManager, swapChain->Width(), swapChain->Height());
		}
	}

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChain->GetBackRTVBuffer().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	commandList->ResourceBarrier(1, &barrier);
	ThrowIfFailed(commandList->Close());

	ID3D12CommandList* cmdListArr[] = { commandList.Get() };
	queue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	swapChain->Present();

	WaitSync();

	swapChain->SwapIndex();

	// Mark that first frame is complete
	if (isFirstFrame)
		isFirstFrame = false;
}

void CommandQueue::FlushResourceQueue()
{
	resourceCommandList->Close();

	ID3D12CommandList* commandListArray[] = {resourceCommandList.Get()};
	queue->ExecuteCommandLists(_countof(commandListArray), commandListArray);

	WaitSync();

	resourceCommandAllocator->Reset();
	resourceCommandList->Reset(resourceCommandAllocator.Get(), nullptr);
}

void CommandQueue::WaitSync()
{
	fenceValue++;
	queue->Signal(fence.Get(), fenceValue);

	if (fence->GetCompletedValue() < fenceValue)
	{
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		::WaitForSingleObject(fenceEvent, INFINITE);
	}
}
