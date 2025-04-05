#include "pch.h"
#include "Texture2DAsset.h"
#include "Graphics.h"
#include "Graphics/GraphicsFunc.h"

namespace flaw {
	Texture2DAsset::Texture2DAsset() {
		_descriptor = {};
	}

	Texture2DAsset::Texture2DAsset(const uint8_t* data, uint32_t width, uint32_t height, PixelFormat format) {
		_descriptor = {};

		_descriptor.format = format;
		_descriptor.width = width;
		_descriptor.height = height;
		_descriptor.usage = UsageFlag::Static;
		_descriptor.bindFlags = BindFlag::ShaderResource;

		uint32_t size = width * height * GetSizePerPixel(format);
		_data.resize(size);
		std::memcpy(_data.data(), data, size);
	}

	void Texture2DAsset::Load() {
		_descriptor.data = _data.data();
		_texture = Graphics::GetGraphicsContext().CreateTexture2D(_descriptor);
	}

	void Texture2DAsset::Unload() {
		_texture.reset();
	}
}