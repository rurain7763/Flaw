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

	class AssetDatabase {
	public:
		static void Init(Application& application);
		static void Cleanup();

		static void Refresh(const char* folderPath, bool recursive);

		static bool GetAssetMetadata(const char* assetFile, AssetMetadata& outMetaData);

		static const std::filesystem::path& GetContentsDirectory();

		static bool ImportAsset(const char* srcPath, const char* destPath);

	private:
		static void RegisterAssetsInFolder(const char* folderPath, bool recursive = true);

		static bool IsValidExtension(const std::filesystem::path& extension);
		static Ref<Asset> CreateAsset(const AssetMetadata& metaData, const std::vector<int8_t>& assetData);

		static bool CreateAssetFile(const char* srcPath, const char* destPath, AssetMetadata& outMetaData, Ref<Asset>& outAsset);

		static bool WriteAssetFile(const char* assetFile, const AssetMetadata& metaData, const std::vector<int8_t>& assetData);
		static bool ParseAssetFile(const char* assetFile, AssetMetadata& outMetaData, std::vector<int8_t>& outAssetData);
	};
}