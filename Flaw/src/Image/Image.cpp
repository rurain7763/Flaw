#include "pch.h"
#include "Image.h"
#include "Log/Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace flaw {
	Image::Image(const char* filePath) {
		_type = GetImageType(filePath);

		if (!Load(filePath, _data, _width, _height, _channels)) {
			Log::Error("Failed to load image : %s", filePath);
			return;
		}
	}

	bool Image::Load(const char* filePath, std::vector<uint8_t>& outData, int32_t& outWidth, int32_t& outHeight, int32_t& outChannels) {
		uint8_t* data = stbi_load(filePath, &outWidth, &outHeight, &outChannels, 0);

		if (data == nullptr) {
			return false;
		}

		outData.resize(outWidth * outHeight * outChannels);
		memcpy(outData.data(), data, outData.size());

		stbi_image_free(data);

		return true;
	}

	Image::Type Image::GetImageType(const char* filePath) {
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
