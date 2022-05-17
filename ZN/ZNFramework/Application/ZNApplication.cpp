#include "ZNApplication.h"

using namespace ZNFramework;

ZNFramework::ZNApplication::ZNApplication()
    :context(nullptr)
{
}

ZNFramework::ZNApplication::~ZNApplication()
{
}

int ZNFramework::ZNApplication::Run()
{
    return 0;
}


//#include "ZNApplication.h"
//#include <windows.h>
//#include <windowsx.h>
//
//using Microsoft::WRL::ComPtr;
//using namespace std;
//using namespace ZNFramework;
//
//LRESULT CALLBACK
//MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	return ZNApplication::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
//}
//
//ZNApplication* ZNApplication::mainApp = nullptr;
//ZNApplication* ZNApplication::GetApp()
//{
//	return mainApp;
//}
//
//ZNApplication::ZNApplication(HINSTANCE hInstance)
//	: hInstance(hInstance)
//{
//	// Only one D3DApp can be constructed.
//	assert(mainApp == nullptr);
//	mainApp = this;
//}
//
//ZNApplication::~ZNApplication()
//{
//	if (graphicDevice != nullptr)
//		FlushCommandQueue();
//}
//
//HINSTANCE ZNApplication::AppInst()const
//{
//	return hInstance;
//}
//
//HWND ZNApplication::MainWnd()const
//{
//	return hwnd;
//}
//
//float ZNApplication::AspectRatio()const
//{
//	return static_cast<float>(width) / height;
//}
//
//bool ZNApplication::Get4xMsaaState()const
//{
//	return _4xMsaaState;
//}
//
//void ZNApplication::Set4xMsaaState(bool value)
//{
//	if (_4xMsaaState != value)
//	{
//		_4xMsaaState = value;
//
//		// Recreate the swapchain and buffers with new multisample settings.
//		CreateSwapChain();
//		OnResize();
//	}
//}
//
//int ZNApplication::Run()
//{
//	MSG msg = { 0 };
//
//	timer.Reset();
//
//	while (msg.message != WM_QUIT)
//	{
//		// 메시지가 있다면 처리, 
//		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) 
//		{
//			TranslateMessage(&msg);
//			DispatchMessageW(&msg);
//		}
//		// 없다면 작업 수행
//		else
//		{
//			timer.Tick();
//
//			if (!isPaused)
//			{
//				CalculateFrameStats();
//				Update(timer);
//				Draw(timer);
//			}
//			else
//			{
//				Sleep(100);
//			}
//		}
//	}
//
//	return (int)msg.wParam;
//}
//
//bool ZNApplication::Initialize()
//{
//	if (!InitMainWindow())
//		return false;
//
//	if (!InitDirect3D())
//		return false;
//
//	OnResize();
//
//	return true;
//}
//
//// RTV, DSV 서술자 힙 생성 - 각 1개
//void ZNApplication::CreateRtvAndDsvDescriptorHeaps()
//{
//	// RTV
//	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
//	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;  // 개수는 스왑체인 버퍼 수 만큼
//	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
//	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//	rtvHeapDesc.NodeMask = 0;
//	ThrowIfFailed(graphicDevice->CreateDescriptorHeap(
//		&rtvHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf())));
//
//	// DSV
//	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
//	dsvHeapDesc.NumDescriptors = 1;
//	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
//	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//	dsvHeapDesc.NodeMask = 0;
//	ThrowIfFailed(graphicDevice->CreateDescriptorHeap(
//		&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.GetAddressOf())));
//}
//
//void ZNApplication::OnResize()
//{
//	assert(graphicDevice);
//	assert(swapChain);
//	assert(commandAllocator);
//
//	// 수행하기 전 기존 커맨드 flush
//	FlushCommandQueue();
//
//	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
//
//	// 자원 해제
//	for (int i = 0; i < SwapChainBufferCount; ++i)
//		swapChainBuffer[i].Reset();
//	depthStencilBuffer.Reset();
//
//	// Resize the swap chain.
//	ThrowIfFailed(swapChain->ResizeBuffers(
//		SwapChainBufferCount,
//		width, height,
//		backBufferFormat,
//		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
//
//	currBackBuffer = 0;
//
//	// Create RTV(Render Target View)
//	// swap chain의 버퍼에 대해서 rtv 생성
//	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
//	for (UINT i = 0; i < SwapChainBufferCount; i++)
//	{
//		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffer[i]))); // swap chain의 버퍼 얻어와서
//		graphicDevice->CreateRenderTargetView(swapChainBuffer[i].Get(), nullptr, rtvHeapHandle); // 그 버퍼의 RTV 생성
//		rtvHeapHandle.Offset(1, rtvDescriptorSize); // 힙의 다음 항목으로 넘어감
//	}
//
//	// Create the depth/stencil buffer and view.
//	D3D12_RESOURCE_DESC depthStencilDesc;
//	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	depthStencilDesc.Alignment = 0;
//	depthStencilDesc.Width = width;
//	depthStencilDesc.Height = height;
//	depthStencilDesc.DepthOrArraySize = 1;
//	depthStencilDesc.MipLevels = 1;
//	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
//	depthStencilDesc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
//	depthStencilDesc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
//	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
//	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
//
//	D3D12_CLEAR_VALUE optClear;  // 자원 지우기 값
//	optClear.Format = depthStencilFormat;
//	optClear.DepthStencil.Depth = 1.0f;
//	optClear.DepthStencil.Stencil = 0;
//	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
//	ThrowIfFailed(graphicDevice->CreateCommittedResource(
//		&heapProperties,
//		D3D12_HEAP_FLAG_NONE,
//		&depthStencilDesc,
//		D3D12_RESOURCE_STATE_COMMON,
//		&optClear,
//		IID_PPV_ARGS(depthStencilBuffer.GetAddressOf())));
//
//	// Create descriptor to mip level 0 of entire resource using the format of the resource.
//	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
//	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
//	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
//	dsvDesc.Format = depthStencilFormat;
//	dsvDesc.Texture2D.MipSlice = 0;
//	graphicDevice->CreateDepthStencilView(depthStencilBuffer.Get(), &dsvDesc, DepthStencilView());
//
//	// 자원 전이: 초기상태 -> 깊이 버퍼
//	auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
//		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
//	commandList->ResourceBarrier(1, &resourceBarrier);
//
//	// Execute the resize commands.
//	ThrowIfFailed(commandList->Close());
//	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
//	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//	// Wait until resize is complete.
//	FlushCommandQueue();
//
//	// viewport 설정
//	viewport.TopLeftX = 0; 
//	viewport.TopLeftY = 0;
//	viewport.Width = static_cast<float>(width);
//	viewport.Height = static_cast<float>(height);
//	viewport.MinDepth = 0.0f;
//	viewport.MaxDepth = 1.0f;
//
//	// scissor Rect 설정
//	scissorRect = { 0, 0, width, height };
//}
//
//LRESULT ZNApplication::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	switch (msg)
//	{
//	case WM_ACTIVATE:  // window가 활성화 상태인지 - 비활성화 시 활성화 될때까지 pause
//		if (LOWORD(wParam) == WA_INACTIVE)
//		{
//			isPaused = true;
//			timer.Stop();
//		}
//		else
//		{
//			isPaused = false;
//			timer.Start();
//		}
//		return 0;
//
//	case WM_SIZE:  // 윈도우 크기
//		width = LOWORD(lParam);
//		height = HIWORD(lParam);
//		if (graphicDevice)
//		{
//			if (wParam == SIZE_MINIMIZED)
//			{
//				isPaused = true;
//				isMinimized = true;
//				isMaximized = false;
//			}
//			else if (wParam == SIZE_MAXIMIZED)
//			{
//				isPaused = false;
//				isMinimized = false;
//				isMaximized = true;
//				OnResize();
//			}
//			else if (wParam == SIZE_RESTORED)  // minimized, maximized 후에 복구 되었을 떄
//			{
//				if (isMinimized)
//				{
//					isPaused = false;
//					isMinimized = false;
//					OnResize();
//				}
//				else if (isMaximized)
//				{
//					isPaused = false;
//					isMaximized = false;
//					OnResize();
//				}
//				else if (isResizing)
//				{
//				}
//				else
//				{
//					OnResize();
//				}
//			}
//		}
//		return 0;
//
//		// resize bar 잡고 있을 때 
//	case WM_ENTERSIZEMOVE:
//		isPaused = true;
//		isResizing = true;
//		timer.Stop();
//		return 0;
//
//		// resize 끝났을때
//	case WM_EXITSIZEMOVE:
//		isPaused = false;
//		isResizing = false;
//		timer.Start();
//		OnResize();
//		return 0;
//
//	case WM_DESTROY:
//		PostQuitMessage(0);
//		return 0;
//
//	case WM_GETMINMAXINFO:
//		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
//		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
//		return 0;
//
//	case WM_LBUTTONDOWN:
//	case WM_MBUTTONDOWN:
//	case WM_RBUTTONDOWN:
//		OnMouseDown(wParam, LOWORD(lParam), HIWORD(lParam));
//		return 0;
//	case WM_LBUTTONUP:
//	case WM_MBUTTONUP:
//	case WM_RBUTTONUP:
//		OnMouseUp(wParam, LOWORD(lParam), HIWORD(lParam));
//		return 0;
//	case WM_MOUSEMOVE:
//		OnMouseMove(wParam, LOWORD(lParam), HIWORD(lParam));
//		return 0;
//	case WM_KEYUP:
//		if (wParam == VK_ESCAPE)
//		{
//			PostQuitMessage(0);
//		}
//		else if ((int)wParam == VK_F2)
//			Set4xMsaaState(!_4xMsaaState);
//
//		return 0;
//	}
//
//	return DefWindowProcW(hwnd, msg, wParam, lParam);
//}
//
//bool ZNApplication::InitMainWindow()
//{
////	if (mhMainWnd)
////		return false;
//
//	WNDCLASSEXW wc = { 0 };
//	wc.cbSize = sizeof(WNDCLASSEXW);
//	wc.style = CS_HREDRAW | CS_VREDRAW;
//	wc.lpfnWndProc = MainWndProc;
//	wc.cbClsExtra = 0;
//	wc.cbWndExtra = 0;
//	wc.hInstance = hInstance;
//	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
//	wc.hCursor = LoadCursor(0, IDC_ARROW);
//	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
//	wc.lpszMenuName = L"asdfadsf";
//	wc.lpszClassName = L"MainWnd";
//
//	if (!RegisterClassExW(&wc))
//	{
//		MessageBoxW(0, L"RegisterClass Failed.", 0, 0);
//		return false;
//	}
//
//	// Compute window rectangle dimensions based on requested client area dimensions.
//    
//	RECT R = { 0, 0, width, height };
//	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
//	int width = R.right - R.left;
//	int height = R.bottom - R.top;
//	
//	hwnd = CreateWindowExW(0, L"MainWnd", CLASS_NAME,
//		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, hInstance, 0);
//
//	if (!hwnd)
//	{
//		MessageBoxW(0, L"CreateWindow Failed.", 0, 0);
//		return false;
//	}
//
//	ShowWindow(hwnd, SW_SHOW);
//	UpdateWindow(hwnd);
//
//	return true;
//}
//
//bool ZNApplication::InitDirect3D()
//{
//#if defined(DEBUG) || defined(_DEBUG) 
//	// Enable the D3D12 debug layer.
//	{
//		ComPtr<ID3D12Debug> debugController;
//		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
//		debugController->EnableDebugLayer();
//	}
//#endif
//
//	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
//
//	// 하드웨어 디바이스 생성
//	HRESULT hardwareResult = D3D12CreateDevice(
//		nullptr,             // default adapter
//		D3D_FEATURE_LEVEL_11_0,
//		IID_PPV_ARGS(&graphicDevice));
//
//	// 실패시 WARP 어댑터 디바이스 생성
//	if (FAILED(hardwareResult))
//	{
//		ComPtr<IDXGIAdapter> pWarpAdapter;
//		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
//
//		ThrowIfFailed(D3D12CreateDevice(
//			pWarpAdapter.Get(),
//			D3D_FEATURE_LEVEL_11_0,
//			IID_PPV_ARGS(&graphicDevice)));
//	}
//
//	// fence 생성, descriptor 크기 얻어옴
//	ThrowIfFailed(graphicDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
//		IID_PPV_ARGS(&fence)));
//
//	rtvDescriptorSize = graphicDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
//	dsvDescriptorSize = graphicDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
//	cbvSrvUavDescriptorSize = graphicDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//
//#ifdef _DEBUG
//	LogAdapters();
//#endif
//
//	CreateCommandObjects();				// command queue, command allocator, command list 
//	CreateSwapChain();					// swap chain
//	CreateRtvAndDsvDescriptorHeaps();	// 
//
//	return true;
//}
//
//void ZNApplication::CreateCommandObjects()
//{
//	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
//	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
//	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
//	ThrowIfFailed(graphicDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
//
//	ThrowIfFailed(graphicDevice->CreateCommandAllocator(
//		D3D12_COMMAND_LIST_TYPE_DIRECT,
//		IID_PPV_ARGS(commandAllocator.GetAddressOf())));
//
//	ThrowIfFailed(graphicDevice->CreateCommandList(
//		0,
//		D3D12_COMMAND_LIST_TYPE_DIRECT,
//		commandAllocator.Get(),
//		nullptr,                   // Initial PipelineStateObject
//		IID_PPV_ARGS(commandList.GetAddressOf())));
//
//	// command list 처음 참조시 Reset() 호출 위해 닫힌 상태로 시작
//	commandList->Close();
//}
//
//void ZNApplication::CreateSwapChain()
//{
//	// 기존 스왑체인 해제
//	swapChain.Reset();
//
//	DXGI_SWAP_CHAIN_DESC sd;
//	sd.BufferDesc.Width = width;
//	sd.BufferDesc.Height = height;
//	sd.BufferDesc.RefreshRate.Numerator = 60;
//	sd.BufferDesc.RefreshRate.Denominator = 1;
//	sd.BufferDesc.Format = backBufferFormat;
//	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
//	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
//	sd.SampleDesc.Count = _4xMsaaState ? 4 : 1;
//	sd.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
//
//	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 후면 버퍼를 렌더타겟으로 사용
//	sd.BufferCount = SwapChainBufferCount;  // 사용할 버퍼 개수 - double buffering: 2
//	sd.OutputWindow = hwnd;	// 랜더링 결과 표시할 핸들
//	sd.Windowed = true; // window: true, 전체화면: false
//	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
//	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
//
//	// command queue 이용해서 flush
//	ThrowIfFailed(factory->CreateSwapChain(
//		commandQueue.Get(),
//		&sd,
//		swapChain.GetAddressOf()));
//}
//
//void ZNApplication::FlushCommandQueue()
//{
//	currentFence++;
//
//	ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFence));
//
//	if (fence->GetCompletedValue() < currentFence)
//	{
//		HANDLE eventHandle = CreateEventExW(nullptr, 0, 0, EVENT_ALL_ACCESS);
//
//		ThrowIfFailed(fence->SetEventOnCompletion(currentFence, eventHandle));
//
//		WaitForSingleObject(eventHandle, INFINITE);
//		CloseHandle(eventHandle);
//	}
//}
//
//ID3D12Resource* ZNApplication::CurrentBackBuffer()const
//{
//	return swapChainBuffer[currBackBuffer].Get();
//}
//
//D3D12_CPU_DESCRIPTOR_HANDLE ZNApplication::CurrentBackBufferView()const
//{
//	// 주어진 오프셋에 해당하는 후면 버퍼 RTV 의 핸들 리턴
//	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
//		rtvHeap->GetCPUDescriptorHandleForHeapStart(),	// 첫 핸들
//		currBackBuffer,		// offset
//		rtvDescriptorSize);	// descriptor의 바이트 크기
//}
//
//D3D12_CPU_DESCRIPTOR_HANDLE ZNApplication::DepthStencilView()const
//{
//	return dsvHeap->GetCPUDescriptorHandleForHeapStart();
//}
//
//void ZNApplication::CalculateFrameStats()
//{
//	static int frameCnt = 0;
//	static float timeElapsed = 0.0f;
//
//	frameCnt++;
//
//	// Compute averages over one second period.
//	if ((timer.TotalTime() - timeElapsed) >= 1.0f)
//	{
//		float fps = (float)frameCnt; // fps = frameCnt / 1
//		float mspf = 1000.0f / fps;
//
//		wstring fpsStr = to_wstring(fps);
//		wstring mspfStr = to_wstring(mspf);
//
//		wstring windowText = CLASS_NAME;
//		windowText += 
//			L"    fps: " + fpsStr +
//			L"   mspf: " + mspfStr;
//
//		::SetWindowTextW(hwnd, windowText.c_str());
//
//		// Reset for next average.
//		frameCnt = 0;
//		timeElapsed += 1.0f;
//	}
//}
//
//void ZNApplication::LogAdapters()
//{
//	UINT i = 0;
//	IDXGIAdapter* adapter = nullptr;
//	std::vector<IDXGIAdapter*> adapterList;
//	while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
//	{
//		DXGI_ADAPTER_DESC desc;
//		adapter->GetDesc(&desc);
//
//		std::wstring text = L"***Adapter: ";
//		text += desc.Description;
//		text += L"\n";
//
//		OutputDebugStringW(text.c_str());
//
//		adapterList.push_back(adapter);
//
//		++i;
//	}
//
//	for (size_t i = 0; i < adapterList.size(); ++i)
//	{
//		LogAdapterOutputs(adapterList[i]);
//		ReleaseCom(adapterList[i]);
//	}
//}
//
//void ZNApplication::LogAdapterOutputs(IDXGIAdapter* adapter)
//{
//	UINT i = 0;
//	IDXGIOutput* output = nullptr;
//	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
//	{
//		DXGI_OUTPUT_DESC desc;
//		output->GetDesc(&desc);
//
//		std::wstring text = L"***Output: ";
//		text += desc.DeviceName;
//		text += L"\n";
//		OutputDebugStringW(text.c_str());
//
//		LogOutputDisplayModes(output, backBufferFormat);
//
//		ReleaseCom(output);
//
//		++i;
//	}
//}
//
//void ZNApplication::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
//{
//	UINT count = 0;
//	UINT flags = 0;
//
//	// Call with nullptr to get list count.
//	output->GetDisplayModeList(format, flags, &count, nullptr);
//
//	std::vector<DXGI_MODE_DESC> modeList(count);
//	output->GetDisplayModeList(format, flags, &count, &modeList[0]);
//
//	for (auto& x : modeList)
//	{
//		UINT n = x.RefreshRate.Numerator;
//		UINT d = x.RefreshRate.Denominator;
//		std::wstring text =
//			L"Width = " + std::to_wstring(x.Width) + L" " +
//			L"Height = " + std::to_wstring(x.Height) + L" " +
//			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
//			L"\n";
//
//		::OutputDebugStringW(text.c_str());
//	}
//}