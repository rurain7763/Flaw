#include "pch.h"
#include "Platform.h"
#include "Application.h"
#include "Platform/Windows/WindowsContext.h"
#include "Event/EventDispatcher.h"

namespace flaw {
	static Scope<PlatformContext> g_platformContext;

	void Platform::Init(const char* title, int32_t width, int32_t height, EventDispatcher& evnDispatcher) {
	#if _WIN32
		g_platformContext = CreateScope<WindowsContext>(title, width, height, evnDispatcher);
	#else
		#error Flaw only supports Windows!
	#endif
	}

	void Platform::Cleanup() {
		g_platformContext.reset();
	}

	bool Platform::PollEvents() {
		return g_platformContext->PollEvents();
	}

	void Platform::SetCursorVisible(bool visible) {
		g_platformContext->SetCursorVisible(visible);
	}

	void Platform::LockCursor(bool lock) {
		g_platformContext->LockCursor(lock);
	}

	void Platform::GetFrameBufferSize(int32_t& width, int32_t& height) {
		g_platformContext->GetFrameBufferSize(width, height);
	}

	int32_t Platform::GetX() {
		return g_platformContext->GetX();
	}

	int32_t Platform::GetY() {
		return g_platformContext->GetY();
	}

	int32_t Platform::GetWidth() {
		return g_platformContext->GetWidth();
	}

	int32_t Platform::GetHeight() {
		return g_platformContext->GetHeight();
	}

	PlatformContext& Platform::GetPlatformContext() {
		return *g_platformContext;
	}
}