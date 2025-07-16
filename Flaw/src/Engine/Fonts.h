#pragma once

#include "Font/FontsContext.h"
#include "Math/Math.h"

namespace flaw {	
	class Fonts {
	public:
		static void Init();
		static void Cleanup();

		static Ref<Font> CreateFontFromFile(const char* filePath);
		static Ref<Font> CreateFontFromMemory(const int8_t* data, uint64_t size);

		static void GenerateFontMeshs(
			Ref<Font> font, 
			vec2 fontAtlasSize,
			const std::wstring& text, 
			const std::function<void(const vec3& ltPos0, const vec2& ltTex0, const vec3& rtPos0, const vec2& rtTex0, const vec3& rbPos0, const vec2& rbTex0, const vec3& lbPos0, const vec2& lbTex0)>& callback,
			float lineOffset = 0.f, 
			float kerningOffset = 0.f
		);
	};
}