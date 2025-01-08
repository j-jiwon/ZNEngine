#pragma once
#include "../../ZN.h"
#include "../ZNTimer.h"
#include <map>
#include <functional>

namespace ZNFramework
{
	enum class KEY_STATE;
	enum class KEY_TYPE;
	struct MouseEvent;
	struct KeyboardEvent;

	class ZNWindow
	{
	public:
		virtual void Create(uint32 inWidth, uint32 inHeight) = 0;
		virtual void Destroy() = 0;
		virtual void Show() = 0;
		virtual void Hide() = 0;

		virtual uint32 Width() const = 0;
		virtual uint32 Height() const = 0;
		virtual void* PlatformHandle() const = 0;

		using EventHandler = const void*;
		using ResizeEventCallback = std::function<void(uint32, uint32)>;
		using MouseEventCallback = std::function<void(MouseEvent)>;
		using KeyboardEventCallback = std::function<void(KeyboardEvent)>;

		inline void AddEventHandler(EventHandler handler,
			ResizeEventCallback callback = nullptr,
			MouseEventCallback mouseCallback = nullptr,
			KeyboardEventCallback keyboardCallback = nullptr)
		{
			resizeEventHandlers.emplace(handler, callback);
			mouseEventHandlers.emplace(handler, mouseCallback);
			keyboardEventHandlers.emplace(handler, keyboardCallback);
		}

		inline void RemoveEventHandler(EventHandler handler)
		{
			resizeEventHandlers.erase(handler);
			mouseEventHandlers.erase(handler);
			keyboardEventHandlers.erase(handler);
		}

	protected:
		bool isPaused = false;			// is the application paused?
		bool isMinimized = false;		// is the application minimized?
		bool isMaximized = false;		// is the application maximized?
		bool isResizing = false;		// are the resize bars being dragged?
		bool fullscreenState = false;	// fullscreen enabled

		ZNTimer timer;

		std::map<EventHandler, ResizeEventCallback> resizeEventHandlers;
		std::map<EventHandler, MouseEventCallback> mouseEventHandlers;
		std::map<EventHandler, KeyboardEventCallback> keyboardEventHandlers;
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
		template <typename T>
		T* GetAs() const
		{
			if constexpr (std::is_base_of_v<ZNWindow, T>)
			{
				return dynamic_cast<T*>(window);
			}
			else
			{
				static_assert(std::is_same_v<T, void>, "Unsupported type for GetAs");
				return nullptr;
			}
		}
	private:
		ZNWindow* window = nullptr;

		WindowContext() = default;
		~WindowContext() = default;
		WindowContext(const WindowContext&) = delete;
		WindowContext& operator=(const WindowContext&) = delete;
	};
}
