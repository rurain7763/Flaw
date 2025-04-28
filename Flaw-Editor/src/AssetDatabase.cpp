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
						std::filesystem::path absolutePath = g_contentsDir.parent_path() / path.parent_path();
						if (path.extension() == ".asset") {
							g_application->AddTask([absolutePath]() { AssetDatabase::Refresh(absolutePath.generic_string().c_str(), false); });
						}
						else if (!path.has_extension()) {
							g_application->AddTask([absolutePath]() { AssetDatabase::Refresh(absolutePath.generic_string().c_str(), true); });
						}
						Log::Info("AssetDatabase auto refreshed!");
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
			std::filesystem::path assetFile = dir.path();

			if (dir.is_directory()) {
				if (recursive) {
					RegisterAssetsInFolder(assetFile.generic_string().c_str(), recursive);
				}
				continue;
			}

			if (assetFile.extension() != ".asset") {
				continue;
			}

			std::vector<int8_t> fileData;
			if (!FileSystem::ReadFile(assetFile.generic_string().c_str(), fileData)) {
				continue;
			}

			SerializationArchive archive((const int8_t*)fileData.data(), fileData.size());

			AssetMetadata metadata;
			archive >> metadata;

			const uint32_t assetDataOffset = archive.Offset();
			const int8_t* assetDataBegin = archive.Data() + assetDataOffset;
			const uint32_t assetDataSize = archive.RemainingSize();

			uint64_t fileIndex = FileSystem::FileIndex(assetFile.generic_string().c_str());
			if (metadata.fileIndex != fileIndex) {
				// 파일이 강제적으로 복사된 경우임. 메타 데이터를 갱신해야 함.
				metadata.handle.Generate();
				metadata.fileIndex = fileIndex;

				SerializationArchive newArchive;
				newArchive << metadata;
				newArchive.Append(assetDataBegin, assetDataSize);

				if (!FileSystem::MakeFile(assetFile.generic_string().c_str(), newArchive.Data(), newArchive.RemainingSize())) {
					continue;
				}
			}

			if (AssetManager::IsAssetRegistered(metadata.handle)) {
				continue;
			}

			Ref<Asset> asset;
			const std::function<void(std::vector<int8_t>&)> getMemoryFunc = [assetFile, assetDataOffset](std::vector<int8_t>& data) {
				FileSystem::ReadFile(assetFile.generic_string().c_str(), data);
				data = std::vector<int8_t>(data.begin() + assetDataOffset, data.end());
			};

			switch (metadata.type) {
				case AssetType::Texture2D: asset = CreateRef<Texture2DAsset>(getMemoryFunc); break;
				case AssetType::TextureCube: asset = CreateRef<TextureCubeAsset>(getMemoryFunc); break;
				case AssetType::Font: asset = CreateRef<FontAsset>(getMemoryFunc); break;
				case AssetType::Sound: asset = CreateRef<SoundAsset>(getMemoryFunc); break;
			}

			if (!asset) {
				continue;
			}

			g_assetMetadataMap[assetFile.generic_string()] = metadata;
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

	bool AssetDatabase::CreateAsset(const AssetCreateSettings* settings) {
		if (!FileSystem::MakeFile(settings->destPath.c_str())) {
			Log::Error("Failed to create asset file: %s", settings->destPath.c_str());
			return false;
		}

		AssetMetadata meta;
		meta.handle.Generate();
		meta.fileIndex = FileSystem::FileIndex(settings->destPath.c_str());

		SerializationArchive archive;
		if (settings->type == AssetSettingsType::Texture) {
			TextureCreateSettings* textureSettings = (TextureCreateSettings*)settings;

			if (textureSettings->textureType == TextureType::Texture2D) {
				meta.type = AssetType::Texture2D;
				archive << meta;
			}
		}

		settings->writeArchiveFunc(archive);

		if (!FileSystem::WriteFile(settings->destPath.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", settings->destPath.c_str());
			return false;
		}

		return true;
	}

	bool AssetDatabase::ImportAsset(const AssetImportSettings* settings) {
		if (!std::filesystem::exists(settings->srcPath)) {
			Log::Error("File does not exist: %s", settings->srcPath);
			return false;
		}

		std::filesystem::path assetPath(settings->destPath);

		if (!FileSystem::MakeFile(settings->destPath.c_str())) {
			Log::Error("Failed to create asset file: %s", settings->destPath.c_str());
			return false;
		}

		AssetMetadata meta;
		meta.handle.Generate();
		meta.fileIndex = FileSystem::FileIndex(settings->destPath.c_str());

		SerializationArchive archive;
		if (settings->type == AssetSettingsType::Texture) {
			TextureImportSettings* textureSettings = (TextureImportSettings*)settings;

			if (textureSettings->textureType == TextureType::Texture2D) {
				meta.type = AssetType::Texture2D;
				archive << meta;

				Image img(settings->srcPath.c_str(), 4);

				Texture2DAsset::WriteToArchive(
					PixelFormat::RGBA8,
					img.Width(),
					img.Height(),
					textureSettings->usageFlags,
					textureSettings->accessFlags,
					textureSettings->bindFlags,
					img.Data(),
					archive
				);
			}
			else if (textureSettings->textureType == TextureType::TextureCube) {
				meta.type = AssetType::TextureCube;
				archive << meta;

				Image img(settings->srcPath.c_str(), 4);

				archive << PixelFormat::RGBA8;
				archive << img.Width();
				archive << img.Height();
				archive << TextureCube::Layout::HorizontalCross; // TODO: 현재는 가로 크로스만 테스트	
				archive << img.Data();
			}
		}
		else if (settings->type == AssetSettingsType::Font) {
			meta.type = AssetType::Font;
			archive << meta;

			std::vector<int8_t> fontData;
			FileSystem::ReadFile(settings->srcPath.c_str(), fontData);

			archive << fontData;

			Ref<Font> font = Fonts::CreateFontFromFile(settings->srcPath.c_str());

			FontAtlas fontAtlas;
			font->GetAtlas(fontAtlas);

			archive << fontAtlas.width;
			archive << fontAtlas.height;
			archive << fontAtlas.data;
		}
		else if (settings->type == AssetSettingsType::Sound) {
			meta.type = AssetType::Sound;
			archive << meta;

			std::vector<int8_t> soundData;
			FileSystem::ReadFile(settings->srcPath.c_str(), soundData);
			archive << soundData;
		}

		if (!FileSystem::WriteFile(settings->destPath.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", settings->destPath.c_str());
			return false;
		}

		return true;
	}
}