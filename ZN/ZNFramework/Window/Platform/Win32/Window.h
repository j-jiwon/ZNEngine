#pragma once
#include "ZN.h"
#include "ZNFramework.h"
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

		void Create(uint32 inWidth, uint32 inHeight) override;
		void Destroy() override;
		void Show() override;
		void Hide() override;

		uint32 Width() const override { return width; }
		uint32 Height() const override { return height; }
		void* PlatformHandle() const override { return hwnd; };

		void OnMouseEvent(struct MouseEvent mouseEvent);
		void OnMouseMove(struct MouseEvent mouseEvent);
		void OnKeyboardEvent(struct KeyboardEvent keyboardEvent);

		void OnKeyDown() {};
		void OnKeyUp() {};

	protected:
		static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static KEY_TYPE ConverWindowsKeyToKeyType(WPARAM wParam);
		static MOUSE_TYPE GetMouseTypeFromMsg(UINT uMsg);
	private:
		HWND hwnd; // win32
		HINSTANCE hInstance;
		// size
		uint32 width;
		uint32 height;

		// input
		vector<KEY_STATE> states;
		bool keyStates[256] = { false }; // window VK_* 0~255
		POINT mousePos = {};
		ZNVector2 deltaPos;
	};
}
