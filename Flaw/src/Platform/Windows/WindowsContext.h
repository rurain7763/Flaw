#pragma once

#include "Platform/PlatformContext.h"

#define NOMINMAX
#include <Windows.h>

namespace flaw {
	class FAPI WindowsContext : public PlatformContext {
	public:
		WindowsContext(const char* title, int32_t width, int32_t height, EventDispatcher& evnDispatcher);
		~WindowsContext();

		bool PollEvents() override;
		void SetCursorVisible(bool visible) override;
		void LockCursor(bool lock) override;
		void GetFrameBufferSize(int32_t& width, int32_t& height) override;

		int32_t GetX() override { return _x; }
		int32_t GetY() override { return _y; }
		int32_t GetWidth() override { return _width; }
		int32_t GetHeight() override { return _height; }

		void SetUserWndProc(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& userWndProc);

		inline HWND GetWindowHandle() { return _hWnd; }
		inline LRESULT InvokeUserWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
			if (_userWndProc) {
				return std::invoke(_userWndProc, hWnd, message, wParam, lParam);
			}
		}

	private:
		void CalculateFrameBufferSize();

	private:
		EventDispatcher& _eventDispatcher;

		HWND _hWnd;

		int32_t _x, _y;
		int32_t _width, _height;
		int32_t _frameBufferWidth, _frameBufferHeight;

		enum class WindowSizeState {
			Normal,
			Maximized,
			Minimized
		};

		WindowSizeState _windowSizeState;

		std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> _userWndProc;

		friend LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};
}