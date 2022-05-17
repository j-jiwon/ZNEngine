#pragma once
#include "../../ZNFramework.h"
#include <windows.h>
#include <map>
#include <functional>

namespace ZNFramework
{
	class ZNWindow
	{
	public:
		ZNWindow();
		~ZNWindow();

		void Create();
		void Destroy();

		using EventHandler = const void*;
		using ResizeEventCallback = std::function<void(size_t, size_t)>;
		void AddEventHandler(EventHandler handler, ResizeEventCallback callback);
		void RemoveEventHandler(EventHandler handler);

	protected:
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	
	private:
		HWND hwnd;
		size_t width;
		size_t height;

		std::map<EventHandler, ResizeEventCallback> handlers;
	};
}