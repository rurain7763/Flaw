#include "pch.h"
#include "Image.h"
#include "Log/Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace flaw {
	Image::Image(const char* filePath, uint32_t desiredChannels) {
		_type = GetImageTypeFromExtension(filePath);

		uint8_t* data = stbi_load(filePath, &_width, &_height, &_channels, desiredChannels);

		if (data == nullptr) {
			Log::Error("Failed to load image : %s", filePath);
			return;
		}

		if (desiredChannels != 0) {
			_channels = desiredChannels;
		}

		_data.resize(_width * _height * _channels);
		memcpy(_data.data(), data, _data.size());

		stbi_image_free(data);
	}

	Image::Image(Image::Type type, const char* source, size_t size, uint32_t desiredChannels) {
		_type = type;
		uint8_t* data = stbi_load_from_memory((const uint8_t*)source, size, &_width, &_height, &_channels, desiredChannels);

		if (data == nullptr) {
			Log::Error("Failed to load image");
			return;
		}

		if (desiredChannels != 0) {
			_channels = desiredChannels;
		}

		_data.resize(_width * _height * _channels);
		memcpy(_data.data(), data, _data.size());

		stbi_image_free(data);
	}

	Image::Type Image::GetImageTypeFromExtension(const char* filePath) {
		std::filesystem::path path(filePath);

		if (path.extension() == ".png") {
			return Image::Type::Png;
		}
		else if (path.extension() == ".jpg") {
			return Image::Type::Jpg;
		}
		else if (path.extension() == ".bmp") {
			return Image::Type::Bmp;
		}
		else if (path.extension() == ".tga") {
			return Image::Type::Tga;
		}
		else if (path.extension() == ".dds") {
			return Image::Type::Dds;
		}
		else {
			return Image::Type::Unknown;
		}
	}
}
