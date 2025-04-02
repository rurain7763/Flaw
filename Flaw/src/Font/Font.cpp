#include "pch.h"
#include "Font.h"
#include "Log/Log.h"
#include "FontData.h"

namespace flaw {
    // NOTE: From imgui_draw.cpp
    static uint32_t* GetGlyphRangeDefault() {
        static uint32_t range[] = {
            0x0020, 0x00ff,
            0
        };
        return range;
    }

	Font::Font(const char* path) 
		: _data(new FontData)
    {
        msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
		if (!ft) {
			Log::Error("Failed to initialize freetype");
            return;
        }

		// TODO: this must be load from meemory, not from file
        msdfgen::FontHandle* font = loadFont(ft, path);
		if (!font) {
			Log::Error("Failed to load font");
			return;
        }

        _data->geometry = msdf_atlas::FontGeometry(&_data->glyphs);

		uint32_t* glyphRange = GetGlyphRangeDefault();
        msdf_atlas::Charset charset;
        for (uint32_t i = 0; glyphRange[i]; i += 2) {
			for (uint32_t j = glyphRange[i]; j <= glyphRange[i + 1]; ++j) {
                charset.add(j);
			}
        }

        float fontScale = 1.0f;
        int32_t glyphLoaded = _data->geometry.loadCharset(font, fontScale, charset);
		Log::Info("Loaded %d glyphs from font %s", glyphLoaded, path);

		msdf_atlas::TightAtlasPacker packer;
        packer.setPixelRange(2.0);
		packer.setMiterLimit(1.0);
		packer.setScale(40.0f);

		int32_t remaining = packer.pack(_data->glyphs.data(), _data->glyphs.size());
        if (remaining) {
			Log::Error("Failed to pack %d glyphs", remaining);
			return;
        }

        int32_t width, height;
		packer.getDimensions(width, height);
        double scale = packer.getScale();

		#define LCG_MULTIPLIER 6364136223846793005ull
		#define LCG_INCREMENT 1442695040888963407ull
		#define COLORING_SEED 0
		#define THREAD_COUNT 8
		#define ANGLE_THRESHOLD 3.0

		bool expensiveColoring = false;
		if (expensiveColoring) {
			msdf_atlas::Workload([this](int i, int threadNo) -> bool {
				unsigned long long glyphSeed = (LCG_MULTIPLIER * (COLORING_SEED ^ i) + LCG_INCREMENT) * !!COLORING_SEED;
				_data->glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, ANGLE_THRESHOLD, glyphSeed);
				return true;
				}, _data->glyphs.size()).finish(THREAD_COUNT);
		}
		else {
			unsigned long long glyphSeed = COLORING_SEED;
			for (msdf_atlas::GlyphGeometry& glyph : _data->glyphs) {
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
		generator.generate(_data->glyphs.data(), _data->glyphs.size());

        msdfgen::Bitmap<uint8_t, 3> bitmap = (msdfgen::Bitmap<uint8_t, 3>&)generator.atlasStorage();

		_data->atlas.width = bitmap.width();
		_data->atlas.height = bitmap.height();

        _data->atlas.data.resize(bitmap.width() * bitmap.height() * 4);

        uint8_t* dst = _data->atlas.data.data();
        const uint8_t* src = (const uint8_t*)bitmap;

		// RGB -> RGBA
        for (int32_t i = 0; i < bitmap.width() * bitmap.height(); ++i) {
			dst[i * 4 + 0] = src[i * 3 + 0]; // R
			dst[i * 4 + 1] = src[i * 3 + 1]; // G
			dst[i * 4 + 2] = src[i * 3 + 2]; // B
			dst[i * 4 + 3] = 255; // A
        }

        destroyFont(font);
        deinitializeFreetype(ft);
	}

	Font::~Font() {
		delete _data;
	}
}
