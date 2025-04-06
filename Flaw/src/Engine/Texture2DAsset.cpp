#include "pch.h"
#include "Texture2DAsset.h"
#include "Graphics.h"
#include "Graphics/GraphicsFunc.h"

namespace flaw {
	void Texture2DAsset::Load() {
		std::vector<int8_t> data;
		_getMemoryFunc(data);

		SerializationArchive archive(data.data(), data.size());

		Texture2D::Descriptor desc = {};
		archive >> desc.format;
		archive >> desc.width;
		archive >> desc.height;
		archive >> desc.wrapS;
		archive >> desc.wrapT;
		archive >> desc.minFilter;
		archive >> desc.magFilter;
		archive >> desc.usage;
		archive >> desc.access;
		archive >> desc.bindFlags;
		
		std::vector<uint8_t> textureData;
		archive >> textureData;
		desc.data = textureData.data();

		_texture = Graphics::GetGraphicsContext().CreateTexture2D(desc);
	}

	void Texture2DAsset::Unload() {
		_texture.reset();
	}
}