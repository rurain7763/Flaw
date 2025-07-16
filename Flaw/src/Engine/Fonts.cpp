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

	Ref<Font> Fonts::CreateFontFromFile(const char* filePath) {
		return g_fontsContext->CreateFont(filePath);
	}

	Ref<Font> Fonts::CreateFontFromMemory(const int8_t* data, uint64_t size) {
		return g_fontsContext->CreateFont(data, size);
	}

	void Fonts::GenerateFontMeshs(
		Ref<Font> font, 
		vec2 fontAtlasSize,
		const std::wstring& text, 
		const std::function<void(const vec3& ltPos0, const vec2& ltTex0, const vec3& rtPos0, const vec2& rtTex0, const vec3& rbPos0, const vec2& rbTex0, const vec3& lbPos0, const vec2& lbTex0)>& callback,
		float lineOffset, 
		float kerningOffset) 
	{
		float x = 0.0f;
		float y = 0.0f;

		const float texelW = 1.0f / fontAtlasSize.x;
		const float texelH = 1.0f / fontAtlasSize.y;

		FontGlyph glyph;
		for (uint32_t uniChar : text) {
			if (uniChar == L'\r') {
				continue;
			}

			if (uniChar == L'\n') {
				x = 0.0f;
				y -= font->LineHeight() - lineOffset;
				continue;
			}

			if (uniChar == L'\t') {
				if (font->TryGetGlyph(' ', glyph)) {
					x += (glyph.advance + kerningOffset) * 4;
				}
				continue;
			}

			if (uniChar == L' ') {
				if (font->TryGetGlyph(' ', glyph)) {
					x += glyph.advance + kerningOffset;
				}
				continue;
			}

			if (!font->TryGetGlyph(uniChar, glyph)) {
				if (!font->TryGetGlyph('?', glyph)) {
					continue;
				}
			}

			vec2 texcoordMin = vec2(glyph.tl, glyph.tb);
			vec2 texcoordMax = vec2(glyph.tr, glyph.tt);

			texcoordMin *= vec2(texelW, texelH);
			texcoordMax *= vec2(texelW, texelH);

			vec2 quadMin = vec2(glyph.l, glyph.b);
			vec2 quadMax = vec2(glyph.r, glyph.t);

			quadMin += vec2(x, y);
			quadMax += vec2(x, y);

			callback(
				vec3(quadMin.x, quadMax.y, 0.0f), vec2(texcoordMin.x, texcoordMax.y),
				vec3(quadMax.x, quadMax.y, 0.0f), vec2(texcoordMax.x, texcoordMax.y),
				vec3(quadMax.x, quadMin.y, 0.0f), vec2(texcoordMax.x, texcoordMin.y),
				vec3(quadMin.x, quadMin.y, 0.0f), vec2(texcoordMin.x, texcoordMin.y)
			);

			x += glyph.advance + kerningOffset;
		}
	}
}