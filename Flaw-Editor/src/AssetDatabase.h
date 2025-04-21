#pragma once

#include <Flaw.h>
#include <filesystem>

namespace flaw {
	struct AssetMetadata {
		AssetType type;
		AssetHandle handle;
		uint64_t fileIndex;
	};

	template <>
	struct Serializer<AssetMetadata> {
		static void Serialize(SerializationArchive& archive, const AssetMetadata& value) {
			archive << value.type;
			archive << value.handle;
			archive << value.fileIndex;
		}

		static void Deserialize(SerializationArchive& archive, AssetMetadata& value) {
			archive >> value.type;
			archive >> value.handle;
			archive >> value.fileIndex;
		}
	};

	enum class AssetImportSettingsType {
		Texture,
		Font,
		Sound,
	};

	struct AssetImportSettings {
		AssetImportSettingsType type;

		std::string srcPath;
		std::string destPath;
	};

	struct TextureImportSettings : public AssetImportSettings {
		TextureType textureType;

		TextureImportSettings() {
			type = AssetImportSettingsType::Texture;
			textureType = TextureType::Texture2D;
		}
	};

	struct FontImportSettings : public AssetImportSettings {
		FontImportSettings() {
			type = AssetImportSettingsType::Font;
		}
	};	

	struct SoundImportSettings : public AssetImportSettings {
		SoundImportSettings() {
			type = AssetImportSettingsType::Sound;
		}
	};

	class AssetDatabase {
	public:
		static void Init(Application& application);
		static void Cleanup();

		static void Refresh(const char* folderPath, bool recursive);

		static bool GetAssetMetadata(const char* assetFile, AssetMetadata& outMetaData);

		static const std::filesystem::path& GetContentsDirectory();

		static bool ImportAsset(AssetImportSettings* importSettings);

	private:
		static void RegisterAssetsInFolder(const char* folderPath, bool recursive = true);
	};
}