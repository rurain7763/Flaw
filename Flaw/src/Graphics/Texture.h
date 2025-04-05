#pragma once

#include "Core.h"
#include "GraphicsType.h"

namespace flaw {
	class Texture2D {
	public:
		enum class Wrap {
			Repeat,
			MirroredRepeat,
			ClampToEdge,
			ClampToBorder
		};

		enum class Filter {
			Nearest,
			Linear
		};

		struct Descriptor {
			const uint8_t* data;
			PixelFormat format;
			uint32_t width, height;

			Wrap wrapS, wrapT;
			Filter minFilter, magFilter;

			UsageFlag usage;
			uint32_t access;

			uint32_t bindFlags;
		};

		Texture2D() = default;
		virtual ~Texture2D() = default;

		virtual void BindToGraphicsShader(const uint32_t slot = 0) = 0;
		virtual void BindToComputeShader(const BindFlag bindFlag, const uint32_t slot = 0) = 0;

		virtual void Unbind() = 0;

		virtual void Fetch(void* outData, uint32_t size) const = 0;

		virtual void CopyTo(Ref<Texture2D>& target) const = 0;
		virtual void CopyToSub(Ref<Texture2D>& target, uint32_t x, uint32_t y, uint32_t width, uint32_t height) const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
	};
}

