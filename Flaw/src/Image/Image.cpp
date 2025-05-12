#include "pch.h"
#include "Image.h"
#include "Log/Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

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

	void Image::SaveToFile(const char* filePath) const {
		if (_data.empty()) {
			Log::Error("Image data is empty");
			return;
		}

		SaveToFile(filePath, _data.data(), _width, _height, _type, _channels);
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

	void Image::SaveToFile(const char* filePath, const void* data, int32_t width, int32_t height, Image::Type type, int32_t channels) {
		switch (type) {
		case Image::Type::Png:
			stbi_write_png(filePath, width, height, channels, data, width * channels);
			break;
		case Image::Type::Jpg:
			stbi_write_jpg(filePath, width, height, channels, data, 100);
			break;
		case Image::Type::Bmp:
			stbi_write_bmp(filePath, width, height, channels, data);
			break;
		case Image::Type::Tga:
			stbi_write_tga(filePath, width, height, channels, data);
			break;
		case Image::Type::Dds:
			Log::Error("DDS format is not supported for saving");
			break;
		default:
			Log::Error("Unknown image format");
			break;
		}
	}
}
