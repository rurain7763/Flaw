#pragma once

#include "Asset.h"
#include "Font/Font.h"
#include "Graphics/Texture.h"

namespace flaw {
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
}