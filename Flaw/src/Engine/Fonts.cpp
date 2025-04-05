#include "pch.h"
#include "Fonts.h"
#include "Font/MSDF/MSDFFontsContext.h"

namespace flaw {
	static Ref<FontContext> g_fontsContext;

	void Fonts::Init() {
		g_fontsContext = CreateRef<MSDFFontsContext>();
	}

	void Fonts::Cleanup() {
		g_fontsContext.reset();
	}

	Ref<Font> Fonts::CreateFont(const char* filePath) {
		return g_fontsContext->CreateFont(filePath);
	}

	Ref<Font> Fonts::CreateFont(const int8_t* data, uint64_t size) {
		return g_fontsContext->CreateFont(data, size);
	}
}