#include "AssetDatabase.h"
#include "Utils/SerializationArchive.h"
#include "Platform/FileSystem.h"
#include "Platform/FileWatch.h"

#include <fstream>

namespace flaw {
	static Application* g_application;
	static std::filesystem::path g_contentsDir;
	static std::unordered_map<std::filesystem::path, AssetMetadata> g_assetMetadataMap;

	static Scope<filewatch::FileWatch<std::filesystem::path>> g_fileWatch;

	void AssetDatabase::Init(Application& application) {
		g_application = &application;

		auto& projectConfig = Project::GetConfig();
		g_contentsDir = projectConfig.path + "/Contents";

		RegisterAssetsInFolder(GetContentsDirectory().generic_string().c_str());

		// 파일 시스템 감시 시작
		g_fileWatch = CreateScope<filewatch::FileWatch<std::filesystem::path>>(
			g_contentsDir.parent_path(),
			[](const std::filesystem::path& path, const filewatch::Event change_type) {
				switch (change_type) {
					case filewatch::Event::added:
					case filewatch::Event::removed:
					case filewatch::Event::renamed_new:
					{
						std::filesystem::path absolutePath;
						if (path.extension() == ".asset") {
							absolutePath = g_contentsDir.parent_path() / path.parent_path();
							g_application->AddTask([absolutePath]() { AssetDatabase::Refresh(absolutePath.generic_string().c_str(), false); });
						}
						else if (!path.has_extension()) {
							absolutePath = g_contentsDir.parent_path() / path.parent_path();
							Log::Info("Directory changed: %s", absolutePath.generic_string().c_str());
							g_application->AddTask([absolutePath]() { AssetDatabase::Refresh(absolutePath.generic_string().c_str(), true); });
						}
						break;
					}
				}
			}
		);
	}

	void AssetDatabase::Cleanup() {
		g_fileWatch.reset();

		for (auto& [path, metadata] : g_assetMetadataMap) {
			AssetManager::UnregisterAsset(metadata.handle);
		}
	}

	void AssetDatabase::Refresh(const char* folderPath, bool recursive) {
		RegisterAssetsInFolder(folderPath, recursive);

		for (auto it = g_assetMetadataMap.begin(); it != g_assetMetadataMap.end(); ) {
			const auto& path = it->first;
			auto& metadata = it->second;

			if (!recursive && path.parent_path().generic_string() != folderPath) {
				++it;
				continue;
			}
			else if (recursive && path.generic_string().find(folderPath) == std::string::npos) {
				++it;
				continue;
			}

			if (!std::filesystem::exists(path)) {
				AssetManager::UnregisterAsset(metadata.handle);
				it = g_assetMetadataMap.erase(it);
			}
			else {
				++it;
			}
		}
	}

	void AssetDatabase::RegisterAssetsInFolder(const char* folderPath, bool recursive) {
		for (auto& dir : std::filesystem::directory_iterator(folderPath)) {
			std::filesystem::path path = dir.path();

			if (dir.is_directory()) {
				if (recursive) {
					RegisterAssetsInFolder(path.generic_string().c_str(), recursive);
				}
				continue;
			}

			if (path.extension() != ".asset") {
				continue;
			}

			AssetMetadata metadata;
			std::vector<int8_t> assetData;
			if (!ParseAssetFile(path.generic_string().c_str(), metadata, assetData)) {
				continue;
			}

			uint64_t fileIndex = FileSystem::FileIndex(path.generic_string().c_str());
			if (metadata.fileIndex != fileIndex) {
				// 파일이 강제적으로 복사된 경우임. 메타 데이터를 갱신해야 함.
				metadata.handle.Generate();
				metadata.fileIndex = fileIndex;

				if (!WriteAssetFile(path.generic_string().c_str(), metadata, assetData)) {
					continue;
				}
			}

			if (AssetManager::IsAssetRegistered(metadata.handle)) {
				continue;
			}

			Ref<Asset> asset = CreateAsset(metadata, assetData);
			if (!asset) {
				continue;
			}

			g_assetMetadataMap[path.generic_string()] = metadata;
			AssetManager::RegisterAsset(metadata.handle, asset);
		}
	}

	bool AssetDatabase::GetAssetMetadata(const char* assetFile, AssetMetadata& outMetaData) {
		auto it = g_assetMetadataMap.find(assetFile);

		if (it != g_assetMetadataMap.end()) {
			outMetaData = it->second;
			return true;
		}

		return false;
	}

	const std::filesystem::path& AssetDatabase::GetContentsDirectory() {
		return g_contentsDir;
	}

	bool AssetDatabase::ImportAsset(const char* srcPath, const char* destPath) {
		if (!std::filesystem::exists(srcPath)) {
			Log::Error("File does not exist: %s", srcPath);
			return false;
		}

		AssetMetadata metadata;
		Ref<Asset> asset = nullptr;
		if (!CreateAssetFile(srcPath, destPath, metadata, asset)) {
			return false;
		}

		g_assetMetadataMap[destPath] = metadata;
		AssetManager::RegisterAsset(metadata.handle, asset);

		return true;
	}

	Ref<Asset> AssetDatabase::CreateAsset(const AssetMetadata& metaData, const std::vector<int8_t>& assetData) {
		Ref<Asset> asset = nullptr;

		SerializationArchive archive((const int8_t*)assetData.data(), assetData.size());
		switch (metaData.type) {
			case AssetType::Texture2D: {
				auto texture2DAsset = CreateRef<Texture2DAsset>();
				archive >> *texture2DAsset;
				asset = texture2DAsset;
				break;
			}
			case AssetType::Font: {
				auto fontAsset = CreateRef<FontAsset>();
				archive >> *fontAsset;
				asset = fontAsset;
				break;
			}
		}

		return asset;
	}

	bool AssetDatabase::IsValidExtension(const std::filesystem::path& extension) {
		return extension == ".png" 
			|| extension == ".jpg" 
			|| extension == ".ttf";
	}

	bool AssetDatabase::CreateAssetFile(const char* srcPath, const char* destPath, AssetMetadata& outMetaData, Ref<Asset>& outAsset) {
		auto extension = std::filesystem::path(srcPath).extension();

		if (!IsValidExtension(extension)) {
			Log::Error("Invalid file extension: %s", extension.generic_string().c_str());
			return false;
		}

		if (!FileSystem::MakeFile(destPath)) {
			Log::Error("Failed to create asset file: %s", destPath);
			return false;
		}

		outMetaData.fileIndex = FileSystem::FileIndex(destPath);

		SerializationArchive archive;
		if (extension == ".png" || extension == ".jpg") {
			outMetaData.type = AssetType::Texture2D;

			Image img(srcPath);

			PixelFormat format;
			switch (img.Channels()) {
				case 1: format = PixelFormat::R8; break;
				case 3: format = PixelFormat::BGR8; break;
				case 4: format = PixelFormat::RGBA8; break;
			}

			outAsset = CreateRef<Texture2DAsset>(img.Data().data(), img.Width(), img.Height(), format);

			archive << outMetaData;
			archive << *std::static_pointer_cast<Texture2DAsset>(outAsset);
		}
		else if (extension == ".ttf") {
			outMetaData.type = AssetType::Font;

			std::vector<char> fontData;
			FileSystem::ReadFile(srcPath, fontData);

			outAsset = CreateRef<FontAsset>((const int8_t*)fontData.data(), fontData.size());

			archive << outMetaData;
			archive << *std::static_pointer_cast<FontAsset>(outAsset);
		}

		if (!FileSystem::WriteFile(destPath, (const char*)archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", destPath);
			return false;
		}

		return true;
	}

	bool AssetDatabase::WriteAssetFile(const char* assetFile, const AssetMetadata& metaData, const std::vector<int8_t>& assetData) {
		SerializationArchive archive;
		archive << metaData;
		archive.Append(assetData);

		if (!FileSystem::MakeFile(assetFile, (const char*)archive.Data(), archive.RemainingSize())) {
			return false;
		}

		return true;
	}

	bool AssetDatabase::ParseAssetFile(const char* assetFile, AssetMetadata& outMetaData, std::vector<int8_t>& outAssetData) {
		std::vector<char> fileData;
		if (!FileSystem::ReadFile(assetFile, fileData)) {
			return false;
		}

		SerializationArchive archive((const int8_t*)fileData.data(), fileData.size());
		archive >> outMetaData;

		const int8_t* begin = archive.Data() + archive.Offset();
		outAssetData.assign(begin, begin + archive.RemainingSize());

		return true;
	}
}