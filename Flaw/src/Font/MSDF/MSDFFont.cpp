#include "pch.h"
#include "MSDFFont.h"
#include "Log/Log.h"

#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define COLORING_SEED 0
#define THREAD_COUNT 8
#define ANGLE_THRESHOLD 3.0

namespace flaw {
	MSDFFont::MSDFFont(msdfgen::FreetypeHandle* ftHandle, const char* filePath) {
		_fontHandle = msdfgen::loadFont(ftHandle, filePath);
		if (!_fontHandle) {
			throw std::runtime_error("Failed to load font");
		}

		_geometry = msdf_atlas::FontGeometry(&_glyphs);
		
		LoadGlyphs();
	}

	MSDFFont::MSDFFont(msdfgen::FreetypeHandle* ftHandle, const int8_t* data, uint64_t size) {
		_fontHandle = msdfgen::loadFontData(ftHandle, (const msdfgen::byte*)data, size);
		if (!_fontHandle) {
			throw std::runtime_error("Failed to load font");
		}

		_geometry = msdf_atlas::FontGeometry(&_glyphs);

		LoadGlyphs();
	}

	MSDFFont::~MSDFFont() {
		msdfgen::destroyFont(_fontHandle);
	}

	float MSDFFont::LineHeight() const {
		const auto& metrics = _geometry.getMetrics();
		const float fscale = 1.0f / (metrics.ascenderY - metrics.descenderY);

		return metrics.lineHeight * fscale;
	}

	bool MSDFFont::TryGetGlyph(uint32_t codepoint, FontGlyph& fontGlyph) {
		auto glyph = _geometry.getGlyph(codepoint);
		if (!glyph) {
			return false;
		}

		const float fscale = GetFontScale();

		double l, b, r, t;
		glyph->getQuadAtlasBounds(l, b, r, t);
		fontGlyph.tl = l; 
		fontGlyph.tb = b; 
		fontGlyph.tr = r; 
		fontGlyph.tt = t;

		glyph->getQuadPlaneBounds(l, b, r, t);
		fontGlyph.l = l * fscale; 
		fontGlyph.b = b * fscale; 
		fontGlyph.r = r * fscale;
		fontGlyph.t = t * fscale;

		fontGlyph.advance = glyph->getAdvance() * fscale;

		return true;
	}

	float MSDFFont::GetAdvance(uint32_t current, uint32_t next) {
		double advance;
		_geometry.getAdvance(advance, current, next);

		return advance * GetFontScale();
	}

	enum Language {
		English = 0x1,
		Korean = 0x2,
	};

	// NOTE: From imgui_draw.cpp
	static void GetGlyphRange(uint32_t language, std::vector<uint32_t>& outRange) {
		if (language & Language::English) {
			outRange.push_back(0x0020);
			outRange.push_back(0x00ff);
		}

		if (language & Language::Korean) {
			outRange.push_back(0xAC00);
			outRange.push_back(0xD7A3);
		}

		outRange.push_back(0);
	}

	void MSDFFont::LoadGlyphs() {
		std::vector<uint32_t> glyphRange;
		GetGlyphRange(Language::English, glyphRange);

		uint32_t glyphCount = 0;
		msdf_atlas::Charset charset;
		for (uint32_t i = 0; glyphRange[i]; i += 2) {
			for (uint32_t j = glyphRange[i]; j <= glyphRange[i + 1]; ++j) {
				charset.add(j);
				glyphCount++;
			}
		}

		float fontScale = 1.0f;
		int32_t glyphLoaded = _geometry.loadCharset(_fontHandle, fontScale, charset);
		_expensiveColoring = glyphLoaded >= 0xff;

		Log::Info("Loaded %d glyphs from font", glyphLoaded);
	}

	float MSDFFont::GetFontScale() const {
		const auto& metrics = _geometry.getMetrics();
		return 1.0f / (metrics.ascenderY - metrics.descenderY);
	}

	void MSDFFont::GetAtlas(FontAtlas& atlas) {
		msdf_atlas::TightAtlasPacker packer;
		packer.setPixelRange(2.0);
		packer.setMiterLimit(1.0);
		packer.setScale(40.0f);

		int32_t remaining = packer.pack(_glyphs.data(), _glyphs.size());
		if (remaining) {
			Log::Error("Failed to pack %d glyphs", remaining);
			return;
		}

		int32_t width, height;
		packer.getDimensions(width, height);
		double scale = packer.getScale();

		if (_expensiveColoring) {
			msdf_atlas::Workload([this](int i, int threadNo) -> bool {
				unsigned long long glyphSeed = (LCG_MULTIPLIER * (COLORING_SEED ^ i) + LCG_INCREMENT) * !!COLORING_SEED;
				_glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, ANGLE_THRESHOLD, glyphSeed);
				return true;
			}, _glyphs.size()).finish(THREAD_COUNT);
		}
		else {
			unsigned long long glyphSeed = COLORING_SEED;
			for (msdf_atlas::GlyphGeometry& glyph : _glyphs) {
				glyphSeed *= LCG_MULTIPLIER;
				glyph.edgeColoring(msdfgen::edgeColoringInkTrap, ANGLE_THRESHOLD, glyphSeed);
			}
		}

		msdf_atlas::ImmediateAtlasGenerator<float, 3, msdf_atlas::msdfGenerator, msdf_atlas::BitmapAtlasStorage<uint8_t, 3>> generator(width, height);

		msdf_atlas::GeneratorAttributes genAttr;
		genAttr.config.overlapSupport = true;
		genAttr.scanlinePass = true;

		generator.setAttributes(genAttr);
		generator.setThreadCount(THREAD_COUNT);
		generator.generate(_glyphs.data(), _glyphs.size());

		msdfgen::Bitmap<uint8_t, 3> bitmap = (msdfgen::Bitmap<uint8_t, 3>&)generator.atlasStorage();

		atlas.width = bitmap.width();
		atlas.height = bitmap.height();
		atlas.data.resize(bitmap.width() * bitmap.height() * 4);

		uint8_t* dst = atlas.data.data();
		const uint8_t* src = (const uint8_t*)bitmap;

		// RGB -> RGBA
		for (int32_t i = 0; i < bitmap.width() * bitmap.height(); ++i) {
			dst[i * 4 + 0] = src[i * 3 + 0]; // R
			dst[i * 4 + 1] = src[i * 3 + 1]; // G
			dst[i * 4 + 2] = src[i * 3 + 2]; // B
			dst[i * 4 + 3] = 255; // A
		}
	}
}