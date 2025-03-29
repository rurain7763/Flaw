#include "pch.h"
#include "WindowsContext.h"
#include "Platform/PlatformEvents.h"
#include "Input/InputCodes.h"
#include "Log/Log.h"

#include <windowsx.h>
#include <codecvt>

namespace flaw {
	KeyCode TranslateKeyCode(WPARAM wParam) {
		switch (wParam) {
			case VK_ESCAPE: return KeyCode::ESCAPE;
			case 'A': return KeyCode::A;
			case 'B': return KeyCode::B;
			case 'C': return KeyCode::C;
			case 'D': return KeyCode::D;
			case 'E': return KeyCode::E;
			case 'F': return KeyCode::F;
			case 'G': return KeyCode::G;
			case 'H': return KeyCode::H;
			case 'I': return KeyCode::I;
			case 'J': return KeyCode::J;
			case 'K': return KeyCode::K;
			case 'L': return KeyCode::L;
			case 'M': return KeyCode::M;
			case 'N': return KeyCode::N;
			case 'O': return KeyCode::O;
			case 'P': return KeyCode::P;
			case 'Q': return KeyCode::Q;
			case 'R': return KeyCode::R;
			case 'S': return KeyCode::S;
			case 'T': return KeyCode::T;
			case 'U': return KeyCode::U;
			case 'V': return KeyCode::V;
			case 'W': return KeyCode::W;
			case 'X': return KeyCode::X;
			case 'Y': return KeyCode::Y;
			case 'Z': return KeyCode::Z;
			case VK_SPACE: return KeyCode::Space;
			case VK_SHIFT: return KeyCode::LSHIFT;
			case VK_CONTROL: return KeyCode::LCtrl;
			case VK_MENU: return KeyCode::LALT;
			case VK_LEFT: return KeyCode::Left;
			case VK_RIGHT: return KeyCode::Right;
			case VK_UP: return KeyCode::Up;
			case VK_DOWN: return KeyCode::Down;
			case '0': return KeyCode::NUM_0;
			case '1': return KeyCode::Num1;
			case '2': return KeyCode::Num2;
			case '3': return KeyCode::Num3;
			case '4': return KeyCode::NUM_4;
			case '5': return KeyCode::NUM_5;
			case '6': return KeyCode::NUM_6;
			case '7': return KeyCode::NUM_7;
			case '8': return KeyCode::NUM_8;
			case '9': return KeyCode::NUM_9;
		}

		return KeyCode::Count;
	}

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		WindowsContext* context = reinterpret_cast<WindowsContext*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (!context) {
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		if (context->InvokeUserWndProc(hWnd, message, wParam, lParam)) {
			return 0;
		}
		
		switch (message) {
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
		case WM_KEYDOWN:
		{
			// repeat key check
			if (lParam & 0x40000000) break;

			KeyCode keyCode = TranslateKeyCode(wParam);
			if (keyCode != KeyCode::Count) {
				context->_eventDispatcher.Dispatch<KeyPressEvent>(keyCode);
			}
			break;
		}
		case WM_KEYUP:
		{
			KeyCode keyCode = TranslateKeyCode(wParam);
			if (keyCode != KeyCode::Count) {
				context->_eventDispatcher.Dispatch<KeyReleaseEvent>(keyCode);
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			double x = GET_X_LPARAM(lParam);
			double y = GET_Y_LPARAM(lParam);
			context->_eventDispatcher.Dispatch<MouseMoveEvent>(x, y);
			break;
		}
		case WM_LBUTTONDOWN:
		{
			context->_eventDispatcher.Dispatch<MousePressEvent>(MouseButton::Left);
			break;
		}
		case WM_RBUTTONDOWN:
		{
			context->_eventDispatcher.Dispatch<MousePressEvent>(MouseButton::Right);
			break;
		}
		case WM_MBUTTONDOWN:
		{
			context->_eventDispatcher.Dispatch<MousePressEvent>(MouseButton::Middle);
			break;
		}
		case WM_LBUTTONUP:
		{
			context->_eventDispatcher.Dispatch<MouseReleaseEvent>(MouseButton::Left);
			break;
		}
		case WM_RBUTTONUP:
		{
			context->_eventDispatcher.Dispatch<MouseReleaseEvent>(MouseButton::Right);
			break;
		}
		case WM_MBUTTONUP:
		{
			context->_eventDispatcher.Dispatch<MouseReleaseEvent>(MouseButton::Middle);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			int16_t xOffset = GET_WHEEL_DELTA_WPARAM(wParam);
			int16_t yOffset = GET_WHEEL_DELTA_WPARAM(wParam);
			context->_eventDispatcher.Dispatch<MouseScrollEvent>(xOffset / WHEEL_DELTA, yOffset / WHEEL_DELTA);
			break;
		}
		case WM_SIZE:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			context->_width = rect.right - rect.left;
			context->_height = rect.bottom - rect.top;
			context->CalculateFrameBufferSize();

			if (wParam == SIZE_MINIMIZED) {
				context->_windowSizeState = WindowsContext::WindowSizeState::Minimized;
				context->_eventDispatcher.Dispatch<WindowIconifyEvent>(true);
			}
			else if (wParam == SIZE_MAXIMIZED) {
				if (context->_windowSizeState == WindowsContext::WindowSizeState::Minimized) {
					context->_eventDispatcher.Dispatch<WindowIconifyEvent>(false);
				}

				context->_windowSizeState = WindowsContext::WindowSizeState::Maximized;
				context->_eventDispatcher.Dispatch<WindowResizeEvent>(context->_width, context->_height, context->_frameBufferWidth, context->_frameBufferHeight);
			}
			else if (wParam == SIZE_RESTORED) {
				if (context->_windowSizeState == WindowsContext::WindowSizeState::Maximized) {
					context->_eventDispatcher.Dispatch<WindowResizeEvent>(context->_width, context->_height, context->_frameBufferWidth, context->_frameBufferHeight);
				}
				else if (context->_windowSizeState == WindowsContext::WindowSizeState::Minimized) {
					context->_eventDispatcher.Dispatch<WindowIconifyEvent>(false);
				}

				context->_windowSizeState = WindowsContext::WindowSizeState::Normal;
			}
			break;
		}
		case WM_EXITSIZEMOVE:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);

			context->_width = rect.right - rect.left;
			context->_height = rect.bottom - rect.top;
			context->CalculateFrameBufferSize();
			context->_eventDispatcher.Dispatch<WindowResizeEvent>(context->_width, context->_height, context->_frameBufferWidth, context->_frameBufferHeight);
			break;
		}
		case WM_KILLFOCUS:
		{
			context->_eventDispatcher.Dispatch<WindowFocusEvent>(false);
			break;
		}
		case WM_SETFOCUS:
		{
			context->_eventDispatcher.Dispatch<WindowFocusEvent>(true);
			break;
		}
		case WM_MOVE:
		{
			context->_x = GET_X_LPARAM(lParam);
			context->_y = GET_Y_LPARAM(lParam);
			context->_eventDispatcher.Dispatch<WindowMoveEvent>(context->_x, context->_y);
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		return 0;
	}

	WindowsContext::WindowsContext(const char* title, int32_t width, int32_t height, EventDispatcher& evnDispatcher) 
		: _eventDispatcher(evnDispatcher)
	{
		// init hInstance
		HINSTANCE hInstance = GetModuleHandleA(nullptr);

		// Register class
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = L"CLIENT";
		wcex.hIconSm = NULL;

		RegisterClassEx(&wcex);

		_hWnd = CreateWindowEx(
			0,
			L"CLIENT",
			std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(title).c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			width,
			height,
			nullptr,
			nullptr,
			hInstance,
			nullptr
		);

		SetWindowLongPtr(_hWnd, GWLP_USERDATA, (LONG_PTR)this);

		// Get primary monitor resolution
		HMONITOR hMonitor = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = { sizeof(MONITORINFO) };
		GetMonitorInfo(hMonitor, &mi);

		// Center window
		int32_t x = (mi.rcMonitor.right - width) / 2;
		int32_t y = (mi.rcMonitor.bottom - height) / 2;

		// Adjust window size
		RECT rect = { 0, 0, width, height };
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
		SetWindowPos(_hWnd, nullptr, x, y, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_FRAMECHANGED);

		ShowWindow(_hWnd, SW_SHOW);
		UpdateWindow(_hWnd);
		
		_width = width;
		_height = height;
		CalculateFrameBufferSize();

		_windowSizeState = WindowSizeState::Normal;
	}

	WindowsContext::~WindowsContext() {
		DestroyWindow(_hWnd);
		UnregisterClass(L"CLIENT", GetModuleHandleA(nullptr));
	}

	void WindowsContext::CalculateFrameBufferSize() {
		HDC hdc = GetDC(_hWnd);

		const float defaultDPI = 96.0f;

		// Get the DPI scale factor
		float dpiX = static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSX)) / defaultDPI;
		float dpiY = static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSY)) / defaultDPI;

		// Calculate framebuffer size
		_frameBufferWidth = static_cast<int32_t>(_width * dpiX);
		_frameBufferHeight = static_cast<int32_t>(_height * dpiY);

		ReleaseDC(_hWnd, hdc);
	}

	bool WindowsContext::PollEvents() {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);

			if (msg.message == WM_QUIT)
				return false;

			DispatchMessage(&msg);
		}

		return true;
	}

	void WindowsContext::SetUserWndProc(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& userWndProc) {
		_userWndProc = userWndProc;
	}

	void WindowsContext::SetCursorVisible(bool visible) {
		ShowCursor(visible);
	}

	void WindowsContext::LockCursor(bool lock) {
		if (lock) {
			RECT rect;
			GetClientRect(_hWnd, &rect);

			POINT ul;
			ul.x = rect.left;
			ul.y = rect.top;

			POINT lr;
			lr.x = rect.right;
			lr.y = rect.bottom;

			MapWindowPoints(_hWnd, nullptr, &ul, 1);
			MapWindowPoints(_hWnd, nullptr, &lr, 1);

			rect.left = ul.x;
			rect.top = ul.y;

			rect.right = lr.x;
			rect.bottom = lr.y;

			ClipCursor(&rect);
		}
		else {
			ClipCursor(nullptr);
		}
	}

	void WindowsContext::GetFrameBufferSize(int32_t& width, int32_t& height) {
		width = _frameBufferWidth;
		height = _frameBufferHeight;
	}
}
