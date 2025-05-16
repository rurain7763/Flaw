#include "AssetDatabase.h"
#include "Utils/SerializationArchive.h"
#include "Platform/FileSystem.h"
#include "Platform/FileWatch.h"
#include "Model/Model.h"

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
				metadata.handle = AssetManager::GenerateNewAssetHandle();
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
				case AssetType::Texture2DArray: asset = CreateRef<Texture2DArrayAsset>(getMemoryFunc); break;
				case AssetType::Font: asset = CreateRef<FontAsset>(getMemoryFunc); break;
				case AssetType::Sound: asset = CreateRef<SoundAsset>(getMemoryFunc); break;
				case AssetType::Mesh: asset = CreateRef<MeshAsset>(getMemoryFunc); break;
				case AssetType::SkeletalMesh:
					asset = CreateRef<SkeletalMeshAsset>(
						[assetFile, assetDataOffset](SkeletalMeshAsset::Descriptor& desc) {
							std::vector<int8_t> data;
							FileSystem::ReadFile(assetFile.generic_string().c_str(), data);
							SerializationArchive archive(&data[assetDataOffset], data.size() - assetDataOffset);
							
							archive >> desc.segments;
							archive >> desc.materials;
							archive >> desc.vertices;
							archive >> desc.indices;
						}
					);
					break;
				case AssetType::GraphicsShader:
					asset = CreateRef<GraphicsShaderAsset>(
						[assetFile, assetDataOffset](GraphicsShaderAsset::Descriptor& desc) {
							std::vector<int8_t> data;
							FileSystem::ReadFile(assetFile.generic_string().c_str(), data);
							SerializationArchive archive(&data[assetDataOffset], data.size() - assetDataOffset);

							archive >> desc.shaderCompileFlags;
							archive >> desc.shaderPath;
						}
					);
					break;
				case AssetType::Material:
					asset = CreateRef<MaterialAsset>(
						[assetFile, assetDataOffset](MaterialAsset::Descriptor& desc) {
							std::vector<int8_t> data;
							FileSystem::ReadFile(assetFile.generic_string().c_str(), data);
							SerializationArchive archive(&data[assetDataOffset], data.size() - assetDataOffset);

							archive >> desc.shaderHandle;
							archive >> desc.albedoTexture;
							archive >> desc.normalTexture;
						}
					);
					break;
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

	AssetHandle AssetDatabase::CreateAsset(const AssetCreateSettings* settings) {
		if (settings->type == AssetCreateSettings::Type::Texture2D) {
			return CreateTexture2D((Texture2DCreateSettings*)settings);
		}

		return AssetHandle();
	}

	AssetHandle AssetDatabase::CreateTexture2D(const Texture2DCreateSettings* settings) {
		if (!FileSystem::MakeFile(settings->destPath.c_str())) {
			Log::Error("Failed to create asset file: %s", settings->destPath.c_str());
			return AssetHandle();
		}

		AssetMetadata meta;
		meta.type = AssetType::Texture2D;
		meta.handle = AssetManager::GenerateNewAssetHandle();
		meta.fileIndex = FileSystem::FileIndex(settings->destPath.c_str());

		SerializationArchive archive;

		archive << meta;
		archive << settings->format;
		archive << settings->width;
		archive << settings->height;
		archive << Texture2D::Wrap::ClampToEdge;
		archive << Texture2D::Wrap::ClampToEdge;
		archive << Texture2D::Filter::Linear;
		archive << Texture2D::Filter::Linear;
		archive << settings->usageFlags;
		archive << settings->accessFlags;
		archive << settings->bindFlags;
		archive << settings->data;

		if (!FileSystem::WriteFile(settings->destPath.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", settings->destPath.c_str());
			return AssetHandle();
		}

		return meta.handle;
	}

	AssetHandle AssetDatabase::CreateMaterial(const MaterialCreateSettings* settings) {
		if (!FileSystem::MakeFile(settings->destPath.c_str())) {
			Log::Error("Failed to create asset file: %s", settings->destPath.c_str());
			return AssetHandle();
		}

		AssetMetadata meta;
		meta.type = AssetType::Material;
		meta.handle = AssetManager::GenerateNewAssetHandle();
		meta.fileIndex = FileSystem::FileIndex(settings->destPath.c_str());

		SerializationArchive archive;
		archive << meta;
		archive << settings->shaderHandle;
		archive << settings->albedoTexture;
		archive << settings->normalTexture;

		if (!FileSystem::WriteFile(settings->destPath.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", settings->destPath.c_str());
			return AssetHandle();
		}

		return meta.handle;
	}

	bool AssetDatabase::ImportAsset(const AssetImportSettings* settings) {
		if (settings->type == AssetImportSettings::Type::Texture2D) {
			return ImportTexture2D((Texture2DImportSettings*)settings);
		}
		else if (settings->type == AssetImportSettings::Type::TextureCube) {
			return ImportTextureCube((TextureCubeImportSettings*)settings);
		}
		else if (settings->type == AssetImportSettings::Type::Texture2DArray) {
			return ImportTexture2DArray((Texture2DArrayImportSettings*)settings);
		}
		else if (settings->type == AssetImportSettings::Type::Font) {
			return ImportFont((FontImportSettings*)settings);
		}
		else if (settings->type == AssetImportSettings::Type::Sound) {
			return ImportSound((SoundImportSettings*)settings);
		}
		else if (settings->type == AssetImportSettings::Type::Model) {
			return ImportModel((ModelImportSettings*)settings);
		}
		else if (settings->type == AssetImportSettings::Type::GraphicsShader) {
			return ImportGraphicsShader((GraphicsShaderImportSettings*)settings);
		}

		Log::Error("Unknown asset type: %d", settings->type);
		return false;
	}

	bool AssetDatabase::ImportTexture2D(Texture2DImportSettings* settings) {
		if (!FileSystem::MakeFile(settings->destPath.c_str())) {
			Log::Error("Failed to create asset file: %s", settings->destPath.c_str());
			return false;
		}

		AssetMetadata meta;
		meta.handle = AssetManager::GenerateNewAssetHandle();
		meta.fileIndex = FileSystem::FileIndex(settings->destPath.c_str());

		SerializationArchive archive;
		meta.type = AssetType::Texture2D;
		archive << meta;

		Image img(settings->srcPath.c_str(), 4);

		archive << PixelFormat::RGBA8;
		archive << img.Width();
		archive << img.Height();
		archive << Texture2D::Wrap::ClampToEdge;
		archive << Texture2D::Wrap::ClampToEdge;
		archive << Texture2D::Filter::Linear;
		archive << Texture2D::Filter::Linear;
		archive << settings->usageFlags;
		archive << settings->accessFlags,
		archive << settings->bindFlags;
		archive << img.Data();

		if (!FileSystem::WriteFile(settings->destPath.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", settings->destPath.c_str());
			return false;
		}

		return true;
	}

	bool AssetDatabase::ImportTextureCube(TextureCubeImportSettings* settings) {
		if (!FileSystem::MakeFile(settings->destPath.c_str())) {
			Log::Error("Failed to create asset file: %s", settings->destPath.c_str());
			return false;
		}

		AssetMetadata meta;
		meta.handle = AssetManager::GenerateNewAssetHandle();
		meta.fileIndex = FileSystem::FileIndex(settings->destPath.c_str());

		SerializationArchive archive;
		meta.type = AssetType::TextureCube;
		archive << meta;

		Image img(settings->srcPath.c_str(), 4);
		archive << PixelFormat::RGBA8;
		archive << img.Width();
		archive << img.Height();
		archive << TextureCube::Layout::HorizontalCross; // TODO: 현재는 가로 크로스만 테스트	
		archive << img.Data();

		if (!FileSystem::WriteFile(settings->destPath.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", settings->destPath.c_str());
			return false;
		}

		return true;
	}

	bool AssetDatabase::ImportTexture2DArray(Texture2DArrayImportSettings* settings) {
		if (!FileSystem::MakeFile(settings->destPath.c_str())) {
			Log::Error("Failed to create asset file: %s", settings->destPath.c_str());
			return false;
		}

		AssetMetadata meta;
		meta.handle = AssetManager::GenerateNewAssetHandle();
		meta.fileIndex = FileSystem::FileIndex(settings->destPath.c_str());

		SerializationArchive archive;
		meta.type = AssetType::Texture2DArray;
		archive << meta;

		std::vector<Ref<Texture2D>> textures;
		for (const auto& imagePath : settings->srcPaths) {
			Image img(imagePath.c_str(), 4);

			Texture2D::Descriptor desc = {};
			desc.data = img.Data().data();
			desc.format = PixelFormat::RGBA8;
			desc.width = img.Width();
			desc.height = img.Height();
			desc.usage = settings->usageFlags;
			desc.access = settings->accessFlags;
			desc.bindFlags = settings->bindFlags;

			textures.push_back(Graphics::GetGraphicsContext().CreateTexture2D(desc));
		}

		Texture2DArray::Descriptor desc = {};
		desc.fromMemory = false;
		desc.textures = textures;

		Ref<Texture2DArray> textureArray = Graphics::CreateTexture2DArray(desc);

		std::vector<uint8_t> textureData;
		Graphics::CaptureTextureArray(textureArray, textureData);

		archive << textureArray->GetArraySize();
		archive << textureArray->GetPixelFormat();
		archive << textureArray->GetWidth();
		archive << textureArray->GetHeight();
		archive << textureArray->GetUsage();
		archive << textureArray->GetAccessFlags();
		archive << textureArray->GetBindFlags();
		archive << textureData;

		if (!FileSystem::WriteFile(settings->destPath.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", settings->destPath.c_str());
			return false;
		}

		return true;
	}

	bool AssetDatabase::ImportFont(FontImportSettings* settings) {
		if (!FileSystem::MakeFile(settings->destPath.c_str())) {
			Log::Error("Failed to create asset file: %s", settings->destPath.c_str());
			return false;
		}

		AssetMetadata meta;
		meta.handle = AssetManager::GenerateNewAssetHandle();
		meta.fileIndex = FileSystem::FileIndex(settings->destPath.c_str());

		SerializationArchive archive;
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

		if (!FileSystem::WriteFile(settings->destPath.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", settings->destPath.c_str());
			return false;
		}

		return true;
	}

	bool AssetDatabase::ImportSound(SoundImportSettings* settings) {
		if (!FileSystem::MakeFile(settings->destPath.c_str())) {
			Log::Error("Failed to create asset file: %s", settings->destPath.c_str());
			return false;
		}

		AssetMetadata meta;
		meta.handle = AssetManager::GenerateNewAssetHandle();
		meta.fileIndex = FileSystem::FileIndex(settings->destPath.c_str());

		SerializationArchive archive;
		meta.type = AssetType::Sound;
		archive << meta;

		std::vector<int8_t> soundData;
		FileSystem::ReadFile(settings->srcPath.c_str(), soundData);
		archive << soundData;

		if (!FileSystem::WriteFile(settings->destPath.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", settings->destPath.c_str());
			return false;
		}

		return true;
	}

	bool AssetDatabase::ImportModel(ModelImportSettings* settings) {
		Model model(settings->srcPath.c_str());
		if (!model.IsValid()) {
			Log::Error("Failed to import model: %s", settings->srcPath.c_str());
			return false;
		}

		std::string fileNamePrefix = std::filesystem::path(settings->destPath).filename().generic_string();

		// skeleton mesh
		std::filesystem::path destPath = settings->destPath;
		destPath.replace_filename(fileNamePrefix + "_SkeletalMesh.asset");

		std::string destPathStr = destPath.generic_string();

		if (!FileSystem::MakeFile(destPathStr.c_str())) {
			Log::Error("Failed to create asset file: %s", destPathStr.c_str());
			return false;
		}

		AssetMetadata meta;
		meta.type = AssetType::SkeletalMesh;
		meta.handle = AssetManager::GenerateNewAssetHandle();
		meta.fileIndex = FileSystem::FileIndex(destPathStr.c_str());

		SerializationArchive archive;

		std::unordered_map<Ref<Image>, AssetHandle> loadedImages;
		std::vector<AssetHandle> loadedMaterial(model.GetMaterialCount());
		for (const auto& mesh : model.GetMeshs()) {
			const ModelMaterial& material = model.GetMaterialAt(mesh.materialIndex);

			MaterialCreateSettings matSet = {};
			std::filesystem::path materialPath = settings->destPath;
			materialPath.replace_filename(fileNamePrefix + "_Material_" + std::to_string(mesh.materialIndex) + ".asset");
			matSet.destPath = materialPath.generic_string();
			matSet.shaderHandle = AssetManager::GetHandleByKey("std3d_geometry");
			matSet.renderMode = RenderMode::Opaque;
			matSet.cullMode = CullMode::Back;
			matSet.depthTest = DepthTest::Less;
			matSet.depthWrite = true;

			std::vector<std::pair<std::string, Ref<Image>>> images = {
				{"Diffuse", material.diffuse},
				{"Normal", material.normal},
				{"Specular", material.specular},
				{"Emissive", material.emissive}
			};

			for (const auto& [kindStr, image] : images) {
				if (image) {
					if (loadedImages.find(image) == loadedImages.end()) {
						Texture2DCreateSettings textureSettings = {};

						std::filesystem::path imageAssetPath = settings->destPath;
						imageAssetPath.replace_filename(fileNamePrefix + "_" + kindStr + ".asset");

						textureSettings.destPath = imageAssetPath.generic_string();
						textureSettings.format = PixelFormat::RGBA8;
						textureSettings.width = image->Width();
						textureSettings.height = image->Height();
						textureSettings.usageFlags = UsageFlag::Static;
						textureSettings.bindFlags = BindFlag::ShaderResource;
						textureSettings.accessFlags = 0;
						textureSettings.data = image->Data();

						loadedImages[image] = CreateTexture2D(&textureSettings);
					}

					if (kindStr == "Diffuse") {
						matSet.albedoTexture = loadedImages[image];
					}
					else if (kindStr == "Normal") {
						matSet.normalTexture = loadedImages[image];
					}
				}
			}

			loadedMaterial[mesh.materialIndex] = CreateMaterial(&matSet);
		}

		std::vector<MeshSegment> segments;
		std::vector<AssetHandle> materials;
		for (const auto& mesh : model.GetMeshs()) {
			segments.push_back(MeshSegment{ PrimitiveTopology::TriangleList, mesh.vertexStart, mesh.vertexCount, mesh.indexStart, mesh.indexCount });
			materials.push_back(loadedMaterial[mesh.materialIndex]);
		}

		std::vector<Vertex3D> vertices;
		std::transform(
			model.GetVertices().begin(),
			model.GetVertices().end(),
			std::back_inserter(vertices),
			[](const ModelVertex& vertex) {
				return Vertex3D{
					vertex.position,
					vertex.texCoord,
					vertex.tangent,
					vertex.normal,
					vertex.bitangent
				};
			}
		);

		archive << meta;
		archive << segments;
		archive << materials;
		archive << vertices;
		archive << model.GetIndices();

		if (!FileSystem::WriteFile(destPathStr.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", destPathStr.c_str());
			return false;
		}

		return true;
	}

	bool AssetDatabase::ImportGraphicsShader(const GraphicsShaderImportSettings* settings) {
		if (!FileSystem::MakeFile(settings->destPath.c_str())) {
			Log::Error("Failed to create asset file: %s", settings->destPath.c_str());
			return false;
		}

		AssetMetadata meta;
		meta.handle = AssetManager::GenerateNewAssetHandle();
		meta.fileIndex = FileSystem::FileIndex(settings->destPath.c_str());

		SerializationArchive archive;
		meta.type = AssetType::GraphicsShader;
		archive << meta;

		archive << settings->compileFlags;
		archive << settings->srcPath;

		if (!FileSystem::WriteFile(settings->destPath.c_str(), archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", settings->destPath.c_str());
			return false;
		}

		return true;
	}
}