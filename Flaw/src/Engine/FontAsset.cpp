#include "pch.h"
#include "FontAsset.h"
#include "Graphics.h"
#include "Fonts.h"
#include "Log/Log.h"

namespace flaw {
	void FontAsset::Load() {
		std::vector<int8_t> data;
		_getMemoryFunc(data);

		SerializationArchive archive(data.data(), data.size());

		std::vector<int8_t> fontData;
		archive >> fontData;

		_font = Fonts::CreateFontFromMemory(fontData.data(), fontData.size());

		FontAtlas atlas;
		archive >> atlas.width;
		archive >> atlas.height;
		archive >> atlas.data;

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