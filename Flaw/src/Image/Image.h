#pragma once

#include "Core.h"

#include <vector>

namespace flaw {
	class Image {
	public:
		enum class Type {
			Png,
			Jpg,
			Bmp,
			Tga,
			Dds,
			Unknown
		};

		Image() = default;
		Image(const char* filePath);

		Type ImageType() const { return _type; }
		const std::vector<uint8_t>& Data() const { return _data; }
		int32_t Width() const { return _width; }
		int32_t Height() const { return _height; }
		int32_t Channels() const { return _channels; }

		bool IsValid() const { return !_data.empty(); }

	private:
		static bool Load(const char* filePath, std::vector<uint8_t>& outData, int32_t& outWidth, int32_t& outHeight, int32_t& outChannels);
		Type GetImageType(const char* filePath);

	private:
		Type _type = Type::Unknown;

		std::vector<uint8_t> _data;
		int32_t _width = 0;
		int32_t _height = 0;
		int32_t _channels = 0;
	};
}
