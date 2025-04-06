#pragma once

#include "Asset.h"
#include "Graphics/Texture.h"

namespace flaw {
	class Texture2DAsset : public Asset {
	public:
		Texture2DAsset(const std::function<void(std::vector<int8_t>&)>& getMemoryFunc) : _getMemoryFunc(getMemoryFunc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Texture2D; }

		const Ref<Texture2D>& GetTexture() const { return _texture; }
		bool IsLoaded() const override { return _texture != nullptr; }

	private:
		std::function<void(std::vector<int8_t>&)> _getMemoryFunc;

		Ref<Texture2D> _texture;
	};
}