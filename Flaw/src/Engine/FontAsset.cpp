#include "pch.h"
#include "FontAsset.h"
#include "Graphics.h"
#include "Fonts.h"
#include "Log/Log.h"

namespace flaw {
	FontAsset::FontAsset(const int8_t* data, uint64_t size) {
		_data.resize(size);
		memcpy(_data.data(), data, size);
	}

	void FontAsset::Load() {
		_font = Fonts::CreateFont(_data.data(), _data.size());
		if (!_font) {
			Log::Error("Failed to load font");
			return;
		}

		FontAtlas atlas;
		_font->GetAtlas(atlas);

		Texture2D::Descriptor desc = {};
		desc.width = atlas.width;
		desc.height = atlas.height;
		desc.format = PixelFormat::RGBA8;
		desc.data = atlas.data.data();
		desc.usage = UsageFlag::Static;
		desc.bindFlags = BindFlag::ShaderResource;

		_fontAtlas = Graphics::GetGraphicsContext().CreateTexture2D(desc);
	}

	void FontAsset::Unload() {
		_fontAtlas.reset();
		_font.reset();
	}
}