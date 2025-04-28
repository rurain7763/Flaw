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

	enum class AssetSettingsType {
		Texture,
		Font,
		Sound,
	};

	struct AssetCreateSettings {
		AssetSettingsType type;

		std::function<void(SerializationArchive&)> writeArchiveFunc;
		std::string destPath;
	};

	struct TextureCreateSettings : public AssetCreateSettings {
		TextureType textureType;

		TextureCreateSettings() {
			type = AssetSettingsType::Texture;
			textureType = TextureType::Texture2D;
		}
	};

	struct AssetImportSettings {
		AssetSettingsType type;

		std::string srcPath;
		std::string destPath;
	};

	struct TextureImportSettings : public AssetImportSettings {
		TextureType textureType;
		UsageFlag usageFlags;
		BindFlag bindFlags;
		uint32_t accessFlags;

		TextureImportSettings() {
			type = AssetSettingsType::Texture;
			textureType = TextureType::Texture2D;
			usageFlags = UsageFlag::Static;
			bindFlags = BindFlag::ShaderResource;
			accessFlags = 0;
		}
	};

	struct FontImportSettings : public AssetImportSettings {
		FontImportSettings() {
			type = AssetSettingsType::Font;
		}
	};	

	struct SoundImportSettings : public AssetImportSettings {
		SoundImportSettings() {
			type = AssetSettingsType::Sound;
		}
	};

	class AssetDatabase {
	public:
		static void Init(Application& application);
		static void Cleanup();

		static void Refresh(const char* folderPath, bool recursive);

		static bool GetAssetMetadata(const char* assetFile, AssetMetadata& outMetaData);

		static const std::filesystem::path& GetContentsDirectory();

		static bool CreateAsset(const AssetCreateSettings* settings);

		static bool ImportAsset(const AssetImportSettings* importSettings);

	private:
		static void RegisterAssetsInFolder(const char* folderPath, bool recursive = true);
	};
}