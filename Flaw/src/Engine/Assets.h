#pragma once

#include "Asset.h"
#include "Graphics/Texture.h"
#include "Font/Font.h"
#include "Sound/SoundSource.h"
#include "Graphics/GraphicsBuffers.h"

namespace flaw {
	class Texture2DAsset : public Asset {
	public:
		Texture2DAsset(const std::function<void(std::vector<int8_t>&)>& getMemoryFunc) : _getMemoryFunc(getMemoryFunc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Texture2D; }

		const Ref<Texture2D>& GetTexture() const { return _texture; }
		bool IsLoaded() const override { return _texture != nullptr; }

		static void WriteToArchive(
			PixelFormat format,
			int32_t width,
			int32_t height,
			UsageFlag usage,
			uint32_t access,
			uint32_t bindFlags,
			const std::vector<uint8_t>& data,
			SerializationArchive& archive
		);

	private:
		std::function<void(std::vector<int8_t>&)> _getMemoryFunc;

		Ref<Texture2D> _texture;
	};

	class Texture2DArrayAsset : public Asset {
	public:
		Texture2DArrayAsset(const std::function<void(std::vector<int8_t>&)>& getMemoryFunc) : _getMemoryFunc(getMemoryFunc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Texture2DArray; }

		const Ref<Texture2DArray>& GetTexture() const { return _texture; }
		bool IsLoaded() const override { return _texture != nullptr; }

		static void WriteToArchive(const Ref<Texture2DArray>& textureArray, SerializationArchive& archive);
		static void WriteToArchive(const std::vector<Ref<Texture2D>>& textures, SerializationArchive& archive);

	private:
		std::function<void(std::vector<int8_t>&)> _getMemoryFunc;

		Ref<Texture2DArray> _texture;
	};

	class TextureCubeAsset : public Asset {
	public:
		TextureCubeAsset(const std::function<void(std::vector<int8_t>&)>& getMemoryFunc) : _getMemoryFunc(getMemoryFunc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::TextureCube; }

		const Ref<TextureCube>& GetTexture() const { return _texture; }
		bool IsLoaded() const override { return _texture != nullptr; }

	private:
		std::function<void(std::vector<int8_t>&)> _getMemoryFunc;

		Ref<TextureCube> _texture;
	};

	class FontAsset : public Asset {
	public:
		FontAsset(const std::function<void(std::vector<int8_t>&)>& getMemoryFunc) : _getMemoryFunc(getMemoryFunc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Font; }
		bool IsLoaded() const override { return _font != nullptr; }

		const Ref<Font>& GetFont() const { return _font; }
		const Ref<Texture2D>& GetFontAtlas() const { return _fontAtlas; }

	private:
		std::function<void(std::vector<int8_t>&)> _getMemoryFunc;

		Ref<Font> _font;
		Ref<Texture2D> _fontAtlas;
	};

	class SoundAsset : public Asset {
	public:
		SoundAsset(const std::function<void(std::vector<int8_t>&)>& getMemoryFunc) : _getMemoryFunc(getMemoryFunc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Sound; }
		bool IsLoaded() const override { return _sound != nullptr; }

		const Ref<SoundSource>& GetSoundSource() const { return _sound; }

	private:
		std::function<void(std::vector<int8_t>&)> _getMemoryFunc;

		Ref<SoundSource> _sound;
	};

	class MeshAsset : public Asset {
	public:
		MeshAsset(const std::function<void(std::vector<int8_t>&)>& getMemoryFunc) : _getMemoryFunc(getMemoryFunc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Mesh; }
		bool IsLoaded() const override { return _vertexBuffer != nullptr && _indexBuffer != nullptr; }

		const Ref<VertexBuffer>& GetVertexBuffer() const { return _vertexBuffer; }
		const Ref<IndexBuffer>& GetIndexBuffer() const { return _indexBuffer; }

	private:
		std::function<void(std::vector<int8_t>&)> _getMemoryFunc;

		Ref<VertexBuffer> _vertexBuffer;
		Ref<IndexBuffer> _indexBuffer;
	};
}