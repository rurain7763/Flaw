#pragma once

#include "Core.h"

namespace flaw {
	struct FontAtlas {
		uint32_t width;
		uint32_t height;
		std::vector<uint8_t> data;
	};

	struct FontGlyph {
		float l, b, r, t; // Local bounds
		float tl, tb, tr, tt; // Texture bounds
		float advance;
	};

	class Font {
	public:
		virtual ~Font() = default;

		virtual float LineHeight() const = 0;

		virtual bool TryGetGlyph(uint32_t codepoint, FontGlyph& fontGlyph) = 0;
		virtual float GetAdvance(uint32_t current, uint32_t next) = 0;

		virtual void GetAtlas(FontAtlas& atlas) = 0;
	};
}