#pragma once

#include "Core.h"

#include "Event/EventDispatcher.h"

namespace flaw {
	class FAPI PlatformContext {
	public:
		virtual ~PlatformContext() = default;

		virtual bool PollEvents() = 0;
		virtual void SetCursorVisible(bool visible) = 0;
		virtual void LockCursor(bool lock) = 0;
		virtual void GetFrameBufferSize(int32_t& width, int32_t& height) = 0;

		virtual int32_t GetX() = 0;
		virtual int32_t GetY() = 0;
		virtual int32_t GetWidth() = 0;
		virtual int32_t GetHeight() = 0;
	};
}