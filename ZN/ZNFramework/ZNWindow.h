#pragma once
#include "../ZNFramework.h"
#include <windows.h>

namespace ZNFramework
{
	class ZNWindow
	{
	public:
		ZNWindow();
		~ZNWindow();

		void Create();

	protected:
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	
	private:
		HWND hwnd;
	};
}