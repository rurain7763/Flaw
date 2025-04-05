#pragma once

#include "Core.h"
#include "Font/Font.h"

namespace flaw {
	class FontContext {
	public:
		FontContext() = default;
		virtual ~FontContext() = default;

		virtual Ref<Font> CreateFont(const char* filePath) = 0;
		virtual Ref<Font> CreateFont(const int8_t* data, uint64_t size) = 0;
	};
}