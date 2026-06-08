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

void ImGuiLayer::Shutdown()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
#endif
