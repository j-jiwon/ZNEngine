#ifdef _WIN32
#include "ImGuiLayer.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "ZNFramework/Graphics/ZNGraphicsContext.h"
#include "ZNFramework/Graphics/Platform/Direct3D12/CommandQueue.h"

using namespace ZNFramework;
using namespace ZNFramework::Platform::Direct3D;

void ImGuiLayer::Init(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue, int bufferCount, DXGI_FORMAT rtvFormat)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 64;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap)));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);

	ImGui_ImplDX12_InitInfo initInfo;
	initInfo.Device           = device;
	initInfo.CommandQueue     = commandQueue;
	initInfo.NumFramesInFlight = bufferCount;
	initInfo.RTVFormat        = rtvFormat;
	initInfo.DSVFormat        = DXGI_FORMAT_UNKNOWN;
	initInfo.SrvDescriptorHeap = srvHeap.Get();
	initInfo.LegacySingleSrvCpuDescriptor = srvHeap->GetCPUDescriptorHandleForHeapStart();
	initInfo.LegacySingleSrvGpuDescriptor = srvHeap->GetGPUDescriptorHandleForHeapStart();
	ImGui_ImplDX12_Init(&initInfo);
}

void ImGuiLayer::BeginFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiLayer::EndFrame()
{
	ImGui::Render();
	CommandQueue* cmdQueue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdQueue->CommandList());
}

ImTextureID ImGuiLayer::AllocateTexture(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srcSrv)
{
	UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE destCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
	destCpu.ptr += (SIZE_T)(descriptorSize * nextTextureSlot);
	device->CopyDescriptorsSimple(1, destCpu, srcSrv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += (UINT64)(descriptorSize * nextTextureSlot);

	nextTextureSlot++;
	return (ImTextureID)gpuHandle.ptr;
}

ImTextureID ImGuiLayer::AllocateGrayscaleTexture(ID3D12Device* device, ID3D12Resource* resource, DXGI_FORMAT format)
{
	UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE destCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
	destCpu.ptr += (SIZE_T)(descriptorSize * nextTextureSlot);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	// Replicate R channel into G and B so single-channel textures display as grayscale
	srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 0, 0, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
	device->CreateShaderResourceView(resource, &srvDesc, destCpu);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += (UINT64)(descriptorSize * nextTextureSlot);

	nextTextureSlot++;
	return (ImTextureID)gpuHandle.ptr;
}

ImTextureID ImGuiLayer::SetTexture(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srcSrv, int slot)
{
	UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE destCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
	destCpu.ptr += (SIZE_T)(descriptorSize * slot);
	device->CopyDescriptorsSimple(1, destCpu, srcSrv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += (UINT64)(descriptorSize * slot);
	return (ImTextureID)gpuHandle.ptr;
}

ImTextureID ImGuiLayer::SetGrayscaleTexture(ID3D12Device* device, ID3D12Resource* resource, DXGI_FORMAT format, int slot)
{
	UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE destCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
	destCpu.ptr += (SIZE_T)(descriptorSize * slot);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 0, 0, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
	device->CreateShaderResourceView(resource, &srvDesc, destCpu);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += (UINT64)(descriptorSize * slot);
	return (ImTextureID)gpuHandle.ptr;
}

void ImGuiLayer::Shutdown()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
#endif
