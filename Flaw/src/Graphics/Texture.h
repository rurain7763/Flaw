#pragma once

#include "Core.h"
#include "GraphicsType.h"

namespace flaw {
	class Texture {
	public:
		enum class Type {
			Texture2D,
		};

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
			Type type;

			const uint8_t* data;
			PixelFormat format;
			uint32_t width, height;

			Wrap wrapS, wrapT;
			Filter minFilter, magFilter;

			UsageFlag usage;
			uint32_t access;

			uint32_t bindFlags;
		};

		Texture() = default;
		virtual ~Texture() = default;

		virtual void BindToGraphicsShader(const uint32_t slot = 0) = 0;
		virtual void BindToComputeShader(const BindFlag bindFlag, const uint32_t slot = 0) = 0;

		virtual void Unbind() = 0;

		virtual void Fetch(void* outData, uint32_t size) const = 0;

		virtual void CopyTo(Ref<Texture>& target) const = 0;
		virtual void CopyToSub(Ref<Texture>& target, uint32_t x, uint32_t y, uint32_t width, uint32_t height) const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
	};
}

