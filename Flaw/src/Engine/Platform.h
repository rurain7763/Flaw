#pragma once

#include "Core.h"
#include "Platform/PlatformContext.h"

namespace flaw {
	class EventDispatcher;

	class Platform {
	public:
		static void Init(const char* title, int32_t width, int32_t height, EventDispatcher& evnDispatcher);
		static void Cleanup();

		static bool PollEvents();
		static void SetCursorVisible(bool visible);
		static void LockCursor(bool lock);
		static void GetFrameBufferSize(int32_t& width, int32_t& height);

		static int32_t GetX();
		static int32_t GetY();
		static int32_t GetWidth();
		static int32_t GetHeight();

		static PlatformContext& GetPlatformContext();
	};
}