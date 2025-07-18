#pragma once

#include "Core.h"
#include "GraphicsType.h"

namespace flaw {
	class Texture {
	public:
		virtual ~Texture() = default;

		virtual ShaderResourceView GetShaderResourceView() const = 0;
		virtual UnorderedAccessView GetUnorderedAccessView() const = 0;
		virtual RenderTargetView GetRenderTargetView(uint32_t mipLevel = 0) const = 0;
		virtual DepthStencilView GetDepthStencilView(uint32_t mipLevel = 0) const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual PixelFormat GetPixelFormat() const = 0;
		virtual UsageFlag GetUsage() const = 0;
		virtual uint32_t GetBindFlags() const = 0;
		virtual uint32_t GetAccessFlags() const = 0;
	};

	class Texture2D : public Texture {
	public:
		// TODO: remove
		enum class Wrap {
			Repeat,
			MirroredRepeat,
			ClampToEdge,
			ClampToBorder
		};

		// TODO: remove
		enum class Filter {
			Nearest,
			Linear
		};

		struct Descriptor {
			const uint8_t* data;
			PixelFormat format;
			uint32_t width, height;

			Wrap wrapS, wrapT; // TODO: remove
			Filter minFilter, magFilter; // TODO: remove

			UsageFlag usage;
			uint32_t access;

			uint32_t bindFlags;
		};

		Texture2D() = default;
		virtual ~Texture2D() = default;

		virtual void GenerateMips(uint32_t level) = 0;

		virtual void Fetch(void* outData, uint32_t size) const = 0;

		virtual void CopyTo(Ref<Texture2D>& target) const = 0;
		virtual void CopyToSub(Ref<Texture2D>& target, uint32_t x, uint32_t y, uint32_t width, uint32_t height) const = 0;
	};

	class Texture2DArray : public Texture {
	public:
		struct Descriptor {
			bool fromMemory = false;

			std::vector<Ref<Texture2D>> textures;

			const uint8_t* data;
			PixelFormat format;
			uint32_t width, height;

			UsageFlag usage;
			uint32_t access;

			uint32_t bindFlags;

			uint32_t arraySize;
		};

		Texture2DArray() = default;
		virtual ~Texture2DArray() = default;

		virtual void FetchAll(void* outData) const = 0;

		virtual void CopyTo(Ref<Texture2DArray>& target) const = 0;

		virtual uint32_t GetArraySize() const = 0;
	};

	class TextureCube : public Texture {
	public:
		enum class Layout {
			Horizontal,
			HorizontalCross,
		};

		struct Descriptor {
			Layout layout;
			const uint8_t* data;
			PixelFormat format;
			uint32_t width, height;

			UsageFlag usage;
			uint32_t access;

			uint32_t bindFlags;
		};

		TextureCube() = default;
		virtual ~TextureCube() = default;

		virtual void GenerateMips(uint32_t level) = 0;
	};
}

