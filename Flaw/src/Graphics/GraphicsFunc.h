#pragma once

#include "Core.h"
#include "GraphicsType.h"
#include "Math/Math.h"

namespace flaw {
	inline uint32_t GetSizePerPixel(const PixelFormat format) {
		switch (format) {
		case PixelFormat::R8:
			return 1;
		case PixelFormat::RG8:
			return 2;
		case PixelFormat::RGB8:
		case PixelFormat::BGR8:
			return 3;
		case PixelFormat::RGBA8:
		case PixelFormat::R32_UINT:
		case PixelFormat::D24S8_UINT:
			return 4;
		case PixelFormat::RGBA32F:
			return 16;
		}

		throw std::runtime_error("Unknown pixel format");
	}

	inline void GetChangedPixelFormat(const PixelFormat srcFormat, const std::vector<uint8_t>& src, const PixelFormat dstFormat, std::vector<uint8_t>& dst) {
		if (srcFormat == dstFormat) {
			dst = src;
			return;
		}

		const uint32_t srcSizePerPixel = GetSizePerPixel(srcFormat);
		const uint32_t dstSizePerPixel = GetSizePerPixel(dstFormat);

		if (src.size() % srcSizePerPixel != 0) {
			throw std::runtime_error("Source data size is not aligned with the source pixel format.");
		}

		const uint32_t pixelCount = src.size() / srcSizePerPixel;
		const uint32_t dstSize = pixelCount * dstSizePerPixel;
		
		dst.resize(dstSize);
		if (srcFormat == PixelFormat::RGB8 && dstFormat == PixelFormat::RGBA8) {
			for (uint32_t i = 0; i < pixelCount; ++i) {
				dst[i * 4 + 0] = src[i * 3 + 0];
				dst[i * 4 + 1] = src[i * 3 + 1];
				dst[i * 4 + 2] = src[i * 3 + 2];
				dst[i * 4 + 3] = 255;
			}
		}
		else if (srcFormat == PixelFormat::RG8 && dstFormat == PixelFormat::RGBA8) {
			for (uint32_t i = 0; i < pixelCount; ++i) {
				uint8_t alpha = src[i * 2 + 1];
				uint8_t gray = alpha ? src[i * 2 + 0] : 0;

				dst[i * 4 + 0] = gray;
				dst[i * 4 + 1] = gray;
				dst[i * 4 + 2] = gray;
				dst[i * 4 + 3] = alpha;
			}
		}
		else {
			throw std::runtime_error(
				"Unsupported pixel format conversion: " +
				std::to_string(static_cast<int>(srcFormat)) + " -> " +
				std::to_string(static_cast<int>(dstFormat))
			);
		}
	}

	inline void GeneratePolygonVertices(float radius, uint32_t segments, std::vector<vec3>& outVec) {
		const float step = 360.0f / segments;
		for (uint32_t i = 0; i < segments; i++) {
			const float radian = glm::radians(i * step);
			outVec.push_back({ radius * cosf(radian), radius * sinf(radian), 0.0f });
		}
	}
}