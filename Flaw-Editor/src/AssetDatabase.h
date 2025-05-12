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

	struct AssetCreateSettings {
		AssetType type;

		std::function<void(SerializationArchive&)> writeArchiveFunc;
		std::string destPath;
	};

	struct Texture2DCreateSettings : public AssetCreateSettings {
		Texture2DCreateSettings() {
			type = AssetType::Texture2D;
		}
	};

	struct AssetImportSettings {
		AssetType type;

		std::string destPath;
	};

	struct Texture2DImportSettings : public AssetImportSettings {
		std::string srcPath;

		UsageFlag usageFlags;
		BindFlag bindFlags;
		uint32_t accessFlags;

		Texture2DImportSettings() {
			type = AssetType::Texture2D;
			usageFlags = UsageFlag::Static;
			bindFlags = BindFlag::ShaderResource;
			accessFlags = 0;
		}
	};

	struct TextureCubeImportSettings : public AssetImportSettings {
		std::string srcPath;

		UsageFlag usageFlags;
		BindFlag bindFlags;
		uint32_t accessFlags;

		TextureCubeImportSettings() {
			type = AssetType::TextureCube;
			usageFlags = UsageFlag::Static;
			bindFlags = BindFlag::ShaderResource;
			accessFlags = 0;
		}
	};

	struct Texture2DArrayImportSettings : public AssetImportSettings {
		std::vector<std::string> srcPaths;

		UsageFlag usageFlags;
		BindFlag bindFlags;
		uint32_t accessFlags;

		Texture2DArrayImportSettings() {
			type = AssetType::Texture2DArray;
			usageFlags = UsageFlag::Static;
			bindFlags = BindFlag::ShaderResource;
			accessFlags = 0;
		}
	};

	struct FontImportSettings : public AssetImportSettings {
		std::string srcPath;

		FontImportSettings() {
			type = AssetType::Font;
		}
	};	

	struct SoundImportSettings : public AssetImportSettings {
		std::string srcPath;

		SoundImportSettings() {
			type = AssetType::Sound;
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