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

        static ZNApplication* mApp;

        HINSTANCE mhAppInst = nullptr; // application instance handle
        HWND      mhMainWnd = nullptr; // main window handle
        bool      mAppPaused = false;  // is the application paused?
        bool      mMinimized = false;  // is the application minimized?
        bool      mMaximized = false;  // is the application maximized?
        bool      mResizing = false;   // are the resize bars being dragged?
        bool      mFullscreenState = false;// fullscreen enabled

        // Set true to use 4X MSAA (?.1.8).  The default is false.
        bool      m4xMsaaState = false;    // 4X MSAA enabled
        UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

        ZNTimer mTimer;

        Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
        Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

        Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
        UINT64 mCurrentFence = 0;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

        static const int SwapChainBufferCount = 2;
        int mCurrBackBuffer = 0;
        Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
        Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

        D3D12_VIEWPORT mScreenViewport;
        D3D12_RECT mScissorRect;

        UINT mRtvDescriptorSize = 0;
        UINT mDsvDescriptorSize = 0;
        UINT mCbvSrvUavDescriptorSize = 0;

        // Derived class should set these in derived constructor to customize starting values.
        std::wstring mMainWndCaption = L"d3d App";
        D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
        DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        int mClientWidth = 800;
        int mClientHeight = 600;
    };
}
