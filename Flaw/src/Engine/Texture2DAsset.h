#pragma once

#include "Asset.h"
#include "Graphics/Texture.h"

namespace flaw {
	class Texture2DAsset : public Asset {
	public:
		Texture2DAsset();
		Texture2DAsset(const uint8_t* data, uint32_t width, uint32_t height, PixelFormat format);

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Texture2D; }

		const Ref<Texture2D>& GetTexture() const { return _texture; }
		bool IsLoaded() const override { return _texture != nullptr; }

	private:
		friend struct Serializer<Texture2DAsset>;

		Texture2D::Descriptor _descriptor;
		std::vector<uint8_t> _data;

		Ref<Texture2D> _texture;
	};

	template <>
	struct Serializer<Texture2DAsset> {
		static void Serialize(SerializationArchive& archive, const Texture2DAsset& value) {
			archive << value._descriptor.format;
			archive << value._descriptor.width;
			archive << value._descriptor.height;
			archive << value._descriptor.wrapS;
			archive << value._descriptor.wrapT;
			archive << value._descriptor.minFilter;
			archive << value._descriptor.magFilter;
			archive << value._descriptor.usage;
			archive << value._descriptor.access;
			archive << value._descriptor.bindFlags;
			archive << value._data;
		}

		static void Deserialize(SerializationArchive& archive, Texture2DAsset& value) {
			archive >> value._descriptor.format;
			archive >> value._descriptor.width;
			archive >> value._descriptor.height;
			archive >> value._descriptor.wrapS;
			archive >> value._descriptor.wrapT;
			archive >> value._descriptor.minFilter;
			archive >> value._descriptor.magFilter;
			archive >> value._descriptor.usage;
			archive >> value._descriptor.access;
			archive >> value._descriptor.bindFlags;
			archive >> value._data;
		}
	};
}