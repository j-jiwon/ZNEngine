#pragma once
#include "../../ZN.h"
#include "../ZNTimer.h"
#include <map>
#include <functional>

namespace ZNFramework
{
	class ZNWindow
	{
	public:
		virtual void Create() = 0;
		virtual void Destroy() = 0;
		virtual void Show() = 0;
		virtual void Hide() = 0;

		virtual uint32_t Width() const = 0;
		virtual uint32_t Height() const = 0;
		virtual void* PlatformHandle() const = 0;

		using EventHandler = const void*;
		using ResizeEventCallback = std::function<void(size_t, size_t)>;

		void AddEventHandler(EventHandler handler, ResizeEventCallback callback)
		{
			handlers.emplace(handler, callback);
		}

		void RemoveEventHandler(EventHandler handler)
		{
			handlers.erase(handler);
		}

	protected:
		bool isPaused = false;			// is the application paused?
		bool isMinimized = false;		// is the application minimized?
		bool isMaximized = false;		// is the application maximized?
		bool isResizing = false;		// are the resize bars being dragged?
		bool fullscreenState = false;	// fullscreen enabled

		ZNTimer timer;

		std::map<EventHandler, ResizeEventCallback> handlers;
	};

	class WindowContext
	{
	public:
		static WindowContext& GetInstance()
		{
			static WindowContext instance;
			return instance;
		}

		void SetWindow(ZNWindow* inWindow)
		{
			window = inWindow;
		}

		ZNWindow* GetWindow() const
		{
			return window;
		}

	private:
		ZNWindow* window = nullptr;

		WindowContext() = default;
		~WindowContext() = default;
		WindowContext(const WindowContext&) = delete;
		WindowContext& operator=(const WindowContext&) = delete;
	};
}
