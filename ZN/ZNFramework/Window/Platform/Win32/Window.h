#pragma once
#include "../../ZNWindow.h"
#include "../../../../ZN.h"
#include <Windows.h>
#include <functional>
#include <map>

using namespace std;

namespace ZNFramework
{
	class Window : public ZNWindow
	{
	public:
		Window();
		~Window() = default;

		void Create() override;
		void Destroy() override;
		void Show() override;
		void Hide() override;
		uint32_t Width() const override { return width; }
		uint32_t Height() const override { return height; }
		void* PlatformHandle() const override { return hwnd; };

	protected:
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		HWND hwnd; // win32
		uint32_t width;
		uint32_t height;
	};
}
