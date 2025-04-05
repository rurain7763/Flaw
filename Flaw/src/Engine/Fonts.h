#pragma once

#include "Font/FontsContext.h"

namespace flaw {
	class Fonts {
	public:
		static void Init();
		static void Cleanup();

		static Ref<Font> CreateFont(const char* filePath);
		static Ref<Font> CreateFont(const int8_t* data, uint64_t size);

		static Ref<FontContext> GetFontsContext();
	};
}