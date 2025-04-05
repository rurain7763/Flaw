#pragma once

#include "Font/Font.h"

#define MSDFGEN_PUBLIC
#include <msdf-atlas-gen/msdf-atlas-gen.h>

namespace flaw {
	class MSDFFont : public Font {
	public:
		MSDFFont(msdfgen::FreetypeHandle* ftHandle, const char* filePath);
		MSDFFont(msdfgen::FreetypeHandle* ftHandle, const int8_t* data, uint64_t size);
		~MSDFFont();

		float LineHeight() const override;

		bool TryGetGlyph(uint32_t codepoint, FontGlyph& fontGlyph) override;

		float GetAdvance(uint32_t current, uint32_t next) override;

		void GetAtlas(FontAtlas& atlas) override;

	private:
		void LoadGlyphs();

		float GetFontScale() const;

	private:
		msdfgen::FontHandle* _fontHandle;

		msdf_atlas::FontGeometry _geometry;
		std::vector<msdf_atlas::GlyphGeometry> _glyphs;

		bool _expensiveColoring;
	};
}