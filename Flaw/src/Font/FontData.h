#pragma once

#include "Core.h"

#define MSDFGEN_PUBLIC
#include <msdf-atlas-gen/msdf-atlas-gen.h>

#include <vector>

namespace flaw {
	struct FontAtlas {
		uint32_t width;
		uint32_t height;
		std::vector<uint8_t> data;
	};

	struct FontData {
		std::vector<msdf_atlas::GlyphGeometry> glyphs;
		msdf_atlas::FontGeometry geometry;

		FontAtlas atlas;
	};
}