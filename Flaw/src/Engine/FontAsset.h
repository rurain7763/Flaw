#pragma once

#include "Asset.h"
#include "Font/Font.h"
#include "Graphics/Texture.h"

namespace flaw {
	class FontAsset : public Asset {
	public:
		FontAsset() = default;
		FontAsset(const int8_t* data, uint64_t size);

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Font; }
		bool IsLoaded() const override { return _font != nullptr; }

		const Ref<Font>& GetFont() const { return _font; }
		const Ref<Texture2D>& GetFontAtlas() const { return _fontAtlas; }

	private:
		friend struct Serializer<FontAsset>;

		std::vector<int8_t> _data;

		Ref<Font> _font;
		Ref<Texture2D> _fontAtlas;
	};

	template <>
	struct Serializer<FontAsset> {
		static void Serialize(SerializationArchive& archive, const FontAsset& value) {
			archive << value._data;
		}

		static void Deserialize(SerializationArchive& archive, FontAsset& value) {
			archive >> value._data;
		}
	};
}