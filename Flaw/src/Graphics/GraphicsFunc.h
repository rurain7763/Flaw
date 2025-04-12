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
			return 3;
		case PixelFormat::RGBA8:
		case PixelFormat::BGRX8:
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

	inline void GenerateCube(std::function<void(vec3, vec2, vec3, vec3, vec3)> vertices, std::vector<uint32_t>& outIndices)
	{
		const vec3 positions[6][4] = {
			// Front (Z-)
			{ {-0.5f,  0.5f, -0.5f}, {0.5f,  0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f} },
			// Right (X+)
			{ {0.5f,  0.5f, -0.5f}, {0.5f,  0.5f,  0.5f}, {0.5f, -0.5f,  0.5f}, {0.5f, -0.5f, -0.5f} },
			// Back (Z+)
			{ {0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f} },
			// Left (X-)
			{ {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f} },
			// Top (Y+)
			{ {-0.5f,  0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}, {0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f} },
			// Bottom (Y-)
			{ {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f} }
		};

		const vec3 normals[6] = {
			{  0,  0, -1 }, // Front
			{  1,  0,  0 }, // Right
			{  0,  0,  1 }, // Back
			{ -1,  0,  0 }, // Left
			{  0,  1,  0 }, // Top
			{  0, -1,  0 }  // Bottom
		};

		const vec3 tangents[6] = {
			{  1,  0,  0 }, // Front
			{  0,  0,  1 }, // Right
			{ -1,  0,  0 }, // Back
			{  0,  0, -1 }, // Left
			{  1,  0,  0 }, // Top
			{  1,  0,  0 }  // Bottom
		};

		const vec2 uvs[4] = {
			{ 0.f, 0.f },
			{ 1.f, 0.f },
			{ 1.f, 1.f },
			{ 0.f, 1.f }
		};

		uint32_t baseIndex = 0;
		for (int face = 0; face < 6; ++face)
		{
			vec3 normal = normals[face];
			vec3 tangent = tangents[face];
			vec3 binormal = normalize(cross(normal, tangent));

			for (int i = 0; i < 4; ++i)
				vertices(positions[face][i], uvs[i], normal, tangent, binormal);

			// Index order same as GenerateCubeMesh (CW winding)
			outIndices.push_back(baseIndex + 0);
			outIndices.push_back(baseIndex + 1);
			outIndices.push_back(baseIndex + 2);
			outIndices.push_back(baseIndex + 0);
			outIndices.push_back(baseIndex + 2);
			outIndices.push_back(baseIndex + 3);

			baseIndex += 4;
		}
	}

	inline void GenerateSphere(std::function<void(vec3, vec2, vec3, vec3, vec3)> vertices, std::vector<uint32_t>& outIndices, uint32_t sectorCount, uint32_t stackCount, float radius = 1.0f) {
		const float PI = 3.14159265359f;

		for (uint32_t i = 0; i <= stackCount; ++i)
		{
			float stackAngle = PI / 2 - i * (PI / stackCount); // +Y(북극) -> -Y(남극)
			float y = radius * sinf(stackAngle);               // Y축 방향
			float r = radius * cosf(stackAngle);               // 수평 반지름

			for (uint32_t j = 0; j <= sectorCount; ++j)
			{
				float sectorAngle = j * (2 * PI / sectorCount); // 0 -> 360도

				float x = r * cosf(sectorAngle);
				float z = r * sinf(sectorAngle);

				vec3 position = { x, y, z };
				vec3 normal = normalize(position);

				// 탄젠트는 경도(sector) 방향의 변화율로 계산
				vec3 tangent = normalize(vec3(-z, 0.0f, x));
				if (length(tangent) < 0.001f)
					tangent = vec3(1.0f, 0.0f, 0.0f); // 극(Pole)에서의 fallback

				vec3 binormal = normalize(cross(normal, tangent));

				vec2 uv = {
					(float)j / sectorCount, // u: 경도
					(float)i / stackCount   // v: 위도
				};

				vertices(position, uv, normal, tangent, binormal);
			}
		}

		for (uint32_t i = 0; i < stackCount; ++i)
		{
			for (uint32_t j = 0; j < sectorCount; ++j)
			{
				uint32_t first = i * (sectorCount + 1) + j;
				uint32_t second = first + sectorCount + 1;

				// Triangle 1 (CW)
				outIndices.push_back(first);
				outIndices.push_back(first + 1);
				outIndices.push_back(second + 1);

				// Triangle 2 (CW)
				outIndices.push_back(first);
				outIndices.push_back(second + 1);
				outIndices.push_back(second);
			}
		}
	}
}