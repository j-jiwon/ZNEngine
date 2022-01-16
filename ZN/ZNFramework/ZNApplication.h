#pragma once
/*
namespace ZNFramework
{
	class ZNApplication
	{
	public:
		ZNApplication();

		void Run();		// process window event
	};
}
*/

#include "./Direct3D12/ZNUtils.h"
#include "ZNTimer.h"
#include "../ZNInclude.h"

namespace ZNFramework
{
    class ZNApplication
    {
    protected:

        ZNApplication(HINSTANCE hInstance);
        ZNApplication(const ZNApplication& rhs) = delete;
        ZNApplication& operator=(const ZNApplication& rhs) = delete;
        virtual ~ZNApplication();

    public:

        static ZNApplication* GetApp();

        HINSTANCE AppInst()const;
        HWND      MainWnd()const;
        float     AspectRatio()const;

        bool Get4xMsaaState()const;
        void Set4xMsaaState(bool value);

        int Run();

        virtual bool Initialize();
        virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    protected:
        virtual void CreateRtvAndDsvDescriptorHeaps();
        virtual void OnResize();
        virtual void Update(const ZNTimer& gt) = 0;
        virtual void Draw(const ZNTimer& gt) = 0;

        // 마우스 입력 추가
        virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
        virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
        virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

    protected:

        bool InitMainWindow();
        bool InitDirect3D();
        void CreateCommandObjects();
        void CreateSwapChain();

        void FlushCommandQueue();

        ID3D12Resource* CurrentBackBuffer()const;
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
        D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

        void CalculateFrameStats();

        void LogAdapters();
        void LogAdapterOutputs(IDXGIAdapter* adapter);
        void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

    protected:

        static ZNApplication* mainApp;

        HINSTANCE hInstance = nullptr;  // application instance handle
        HWND      hwnd = nullptr;       // main window handle

        bool      isPaused = false;     // is the application paused?
        bool      isMinimized = false;  // is the application minimized?
        bool      isMaximized = false;  // is the application maximized?
        bool      isResizing = false;   // are the resize bars being dragged?
        bool      fullscreenState = false;// fullscreen enabled

        // Set true to use 4X MSAA (?.1.8).  The default is false.
        bool      _4xMsaaState = false;    // 4X MSAA enabled
        UINT      _4xMsaaQuality = 0;      // quality level of 4X MSAA

        ZNTimer timer;

        Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
        Microsoft::WRL::ComPtr<ID3D12Device> graphicDevice;

        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        UINT64 currentFence = 0;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

        static const int SwapChainBufferCount = 2; // double buffering
        int currBackBuffer = 0;
        Microsoft::WRL::ComPtr<ID3D12Resource> swapChainBuffer[SwapChainBufferCount];
        Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;

        D3D12_VIEWPORT viewport;
        D3D12_RECT scissorRect;

        UINT rtvDescriptorSize = 0;
        UINT dsvDescriptorSize = 0;
        UINT cbvSrvUavDescriptorSize = 0;

        // Derived class should set these in derived constructor to customize starting values.
        D3D_DRIVER_TYPE d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
        DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        int width = 800;
        int height = 600;
    };
}
