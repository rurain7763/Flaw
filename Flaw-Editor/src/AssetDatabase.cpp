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

		Log::Info("AssetDatabase refreshed!");
	}

	void AssetDatabase::RegisterAssetsInFolder(const char* folderPath, bool recursive) {
		for (auto& dir : std::filesystem::directory_iterator(folderPath)) {
			std::filesystem::path assetFile = dir.path();

			if (dir.is_directory()) {
				// it is directory, go deeper
				if (recursive) {
					RegisterAssetsInFolder(assetFile.generic_string().c_str(), recursive);
				}
				continue;
			}

			if (assetFile.extension() != ".asset") {
				// we are not interested in other file types, skip them
				continue;
			}

			std::vector<int8_t> fileData;
			if (!FileSystem::ReadFile(assetFile.generic_string().c_str(), fileData)) {
				continue;
			}

			SerializationArchive archive(fileData.data(), fileData.size());

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

			Ref<Asset> asset = CreateAssetInstance(metadata.type, assetFile, assetDataOffset);
			if (!asset) {
				continue;
			}

			g_assetMetadataMap[assetFile.generic_string()] = metadata;
			AssetManager::RegisterAsset(metadata.handle, asset);
		}
	}

	Ref<Asset> AssetDatabase::CreateAssetInstance(AssetType assetType, const std::filesystem::path& path, int32_t dataOffset) {
		Ref<Asset> asset;

		switch (assetType) {
		case AssetType::Texture2D:
			asset = CreateRef<Texture2DAsset>(
				[path, dataOffset](Texture2DAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.format;
					archive >> desc.width;
					archive >> desc.height;
					archive.Consume(sizeof(Texture2D::Wrap) * 2);
					archive.Consume(sizeof(Texture2D::Filter) * 2);
					archive >> desc.usage;
					archive >> desc.accessFlags;
					archive >> desc.bindFlags;
					archive >> desc.data;
				}
			);
			break;
		case AssetType::Texture2DArray:
			asset = CreateRef<Texture2DArrayAsset>(
				[path, dataOffset](Texture2DArrayAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.arraySize;
					archive >> desc.format;
					archive >> desc.width;
					archive >> desc.height;
					archive >> desc.usage;
					archive >> desc.accessFlags;
					archive >> desc.bindFlags;
					archive >> desc.data;
				}
			);
			break;
		case AssetType::TextureCube:
			asset = CreateRef<TextureCubeAsset>(
				[path, dataOffset](TextureCubeAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.format;
					archive >> desc.width;
					archive >> desc.height;
					archive >> desc.layout;
					archive >> desc.data;
				}
			);
			break;
		case AssetType::Font:
			asset = CreateRef<FontAsset>(
				[path, dataOffset](FontAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.fontData;
					archive >> desc.width;
					archive >> desc.height;
					archive >> desc.atlasData;
				}
			);
			break;
		case AssetType::Sound:
			asset = CreateRef<SoundAsset>(
				[path, dataOffset](SoundAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.soundData;
				}
			);
			break;
		case AssetType::SkeletalMesh:
			asset = CreateRef<SkeletalMeshAsset>(
				[path, dataOffset](SkeletalMeshAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.segments;
					archive >> desc.skeleton;
					archive >> desc.materials;
					archive >> desc.vertices;
					archive >> desc.indices;
				}
			);
			break;
		case AssetType::GraphicsShader:
			asset = CreateRef<GraphicsShaderAsset>(
				[path, dataOffset](GraphicsShaderAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.shaderCompileFlags;
					archive >> desc.shaderPath;
				}
			);
			break;
		case AssetType::Material:
			asset = CreateRef<MaterialAsset>(
				[path, dataOffset](MaterialAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.renderMode;
					archive >> desc.cullMode;
					archive >> desc.depthTest;
					archive >> desc.depthWrite;
					archive >> desc.shaderHandle;
					archive >> desc.albedoTexture;
					archive >> desc.normalTexture;
					archive >> desc.emissiveTexture;
					archive >> desc.metallicTexture;
					archive >> desc.roughnessTexture;
					archive >> desc.ambientOcclusionTexture;
				}
			);
			break;
		case AssetType::Skeleton:
			asset = CreateRef<SkeletonAsset>(
				[path, dataOffset](SkeletonAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.globalInvMatrix;
					archive >> desc.nodes;
					archive >> desc.boneMap;
					archive >> desc.animationHandles;
				}
			);
			break;
		case AssetType::SkeletalAnimation:
			asset = CreateRef<SkeletalAnimationAsset>(
				[path, dataOffset](SkeletalAnimationAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.name;
					archive >> desc.durationSec;
					archive >> desc.animationNodes;
				}
			);
			break;
		case AssetType::StaticMesh:
			asset = CreateRef<StaticMeshAsset>(
				[path, dataOffset](StaticMeshAsset::Descriptor& desc) {
					std::vector<int8_t> data;
					FileSystem::ReadFile(path.generic_string().c_str(), data);
					SerializationArchive archive(&data[dataOffset], data.size() - dataOffset);
					archive >> desc.segments;
					archive >> desc.materials;
					archive >> desc.vertices;
					archive >> desc.indices;
				}
			);
			break;
		}

		return asset;
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

	AssetHandle AssetDatabase::CreateAssetFile(const char* path, AssetType assetType, std::function<void(SerializationArchive&)> serializeFunc) {
		std::string uniquePath = FileSystem::GetUniqueFilePath(path);
		path = uniquePath.c_str();
		
		if (!FileSystem::MakeFile(path)) {
			Log::Error("Failed to create asset file: %s", path);
			return AssetHandle();
		}

		AssetMetadata meta;
		meta.type = assetType;
		meta.handle = AssetManager::GenerateNewAssetHandle();
		meta.fileIndex = FileSystem::FileIndex(path);

		SerializationArchive archive;
		archive << meta;
		serializeFunc(archive);

		if (!FileSystem::WriteFile(path, archive.Data(), archive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", path);
			return AssetHandle();
		}

		return meta.handle;
	}

	AssetHandle AssetDatabase::RecreateAssetFile(const char* path, AssetType assetType, std::function<void(SerializationArchive&)> serializeFunc) {
		std::vector<int8_t> fileData;
		if (!FileSystem::ReadFile(path, fileData)) {
			return AssetHandle();
		}

		SerializationArchive archive(fileData.data(), fileData.size());

		AssetMetadata metadata;
		archive >> metadata;

		SerializationArchive newArchive;
		newArchive << metadata;
		serializeFunc(newArchive);

		if (!FileSystem::WriteFile(path, newArchive.Data(), newArchive.RemainingSize())) {
			Log::Error("Failed to write asset file: %s", path);
			return AssetHandle();
		}

		return metadata.handle;
	}

	AssetHandle AssetDatabase::CreateAsset(const AssetCreateSettings* settings) {
		if (settings->type == AssetCreateSettings::Type::Texture2D) {
			return CreateAssetFile(settings->destPath.c_str(), AssetType::Texture2D, [settings](SerializationArchive& archive) { FillSerializationArchive(archive, (Texture2DCreateSettings*)settings); });
		}
		else if (settings->type == AssetCreateSettings::Type::Material) {
			return CreateAssetFile(settings->destPath.c_str(), AssetType::Material, [settings](SerializationArchive& archive) { FillSerializationArchive(archive, (MaterialCreateSettings*)settings); });
		}
		else if (settings->type == AssetCreateSettings::Type::Skeleton) {
			return CreateAssetFile(settings->destPath.c_str(), AssetType::Skeleton, [settings](SerializationArchive& archive) { FillSerializationArchive(archive, (SkeletonCreateSettings*)settings); });
		}
		else if (settings->type == AssetCreateSettings::Type::SkeletalAnimation) {
			return CreateAssetFile(settings->destPath.c_str(), AssetType::SkeletalAnimation, [settings](SerializationArchive& archive) { FillSerializationArchive(archive, (SkeletalAnimationCreateSettings*)settings); });
		}

		return AssetHandle();
	}

	void AssetDatabase::RecreateAsset(const char* assetFile, const AssetCreateSettings* settings) {
		if (settings->type == AssetCreateSettings::Type::Texture2D) {
			RecreateAssetFile(assetFile, AssetType::Texture2D, [settings](SerializationArchive& archive) { FillSerializationArchive(archive, (Texture2DCreateSettings*)settings); });
		}
		else if (settings->type == AssetCreateSettings::Type::Material) {
			RecreateAssetFile(assetFile, AssetType::Material, [settings](SerializationArchive& archive) { FillSerializationArchive(archive, (MaterialCreateSettings*)settings); });
		}
		else if (settings->type == AssetCreateSettings::Type::Skeleton) {
			RecreateAssetFile(assetFile, AssetType::Skeleton, [settings](SerializationArchive& archive) { FillSerializationArchive(archive, (SkeletonCreateSettings*)settings); });
		}
		else if (settings->type == AssetCreateSettings::Type::SkeletalAnimation) {
			RecreateAssetFile(assetFile, AssetType::SkeletalAnimation, [settings](SerializationArchive& archive) { FillSerializationArchive(archive, (SkeletalAnimationCreateSettings*)settings); });
		}

		AssetMetadata metadata = g_assetMetadataMap[assetFile];
		AssetManager::UnloadAsset(metadata.handle);
	}

	void AssetDatabase::FillSerializationArchive(SerializationArchive& archive, const Texture2DCreateSettings* settings) {
		archive << settings->format;
		archive << settings->width;
		archive << settings->height;
		archive << Texture2D::Wrap::ClampToEdge; // U
		archive << Texture2D::Wrap::ClampToEdge; // V
		archive << Texture2D::Filter::Linear; // Min filter
		archive << Texture2D::Filter::Linear; // Mag filter
		archive << settings->usageFlags;
		archive << settings->accessFlags;
		archive << settings->bindFlags;
		archive << settings->data;
	}

	void AssetDatabase::FillSerializationArchive(SerializationArchive& archive, const MaterialCreateSettings* settings) {
		archive << settings->renderMode;
		archive << settings->cullMode;
		archive << settings->depthTest;
		archive << settings->depthWrite;
		archive << settings->shaderHandle;
		archive << settings->albedoTexture;
		archive << settings->normalTexture;
		archive << settings->emissiveTexture;
		archive << settings->metallicTexture;
		archive << settings->roughnessTexture;
		archive << settings->ambientOcclusionTexture;
	}

	void AssetDatabase::FillSerializationArchive(SerializationArchive& archive, const SkeletonCreateSettings* settings) {
		archive << settings->globalInvMatrix;
		archive << settings->nodes;
		archive << settings->boneMap;
		archive << settings->animationHandles;
	}

	void AssetDatabase::FillSerializationArchive(SerializationArchive& archive, const SkeletalAnimationCreateSettings* settings) {
		archive << settings->name;
		archive << settings->durationSec;
		archive << settings->animationNodes;
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
		return CreateAssetFile(settings->destPath.c_str(), AssetType::Texture2D, [&](SerializationArchive& archive) {
			Image img(settings->srcPath.c_str(), 4);		
			archive << PixelFormat::RGBA8;
			archive << img.Width();
			archive << img.Height();
			archive << Texture2D::Wrap::ClampToEdge;
			archive << Texture2D::Wrap::ClampToEdge;
			archive << Texture2D::Filter::Linear;
			archive << Texture2D::Filter::Linear;
			archive << settings->usageFlags;
			archive << settings->accessFlags;
			archive << settings->bindFlags;
			archive << img.Data();
		}).IsValid();
	}

	bool AssetDatabase::ImportTextureCube(TextureCubeImportSettings* settings) {
		return CreateAssetFile(settings->destPath.c_str(), AssetType::TextureCube, [&](SerializationArchive& archive) {
			Image img(settings->srcPath.c_str(), 4);
			archive << PixelFormat::RGBA8;
			archive << img.Width();
			archive << img.Height();
			archive << TextureCube::Layout::HorizontalCross; // TODO: 현재는 가로 크로스만 테스트
			archive << img.Data();
		}).IsValid();
	}

	bool AssetDatabase::ImportTexture2DArray(Texture2DArrayImportSettings* settings) {
		return CreateAssetFile(settings->destPath.c_str(), AssetType::Texture2DArray, [&](SerializationArchive& archive) {
			Texture2DArray::Descriptor arrayDesc = {};
			arrayDesc.fromMemory = false;
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

				arrayDesc.textures.push_back(Graphics::GetGraphicsContext().CreateTexture2D(desc));
			}		

			Ref<Texture2DArray> textureArray = Graphics::CreateTexture2DArray(arrayDesc);

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
		}).IsValid();
	}

	bool AssetDatabase::ImportFont(FontImportSettings* settings) {
		return CreateAssetFile(settings->destPath.c_str(), AssetType::Font, [&](SerializationArchive& archive) {
			std::vector<int8_t> fontData;
			FileSystem::ReadFile(settings->srcPath.c_str(), fontData);

			Ref<Font> font = Fonts::CreateFontFromFile(settings->srcPath.c_str());

			FontAtlas fontAtlas;
			font->GetAtlas(fontAtlas);

			archive << fontData;
			archive << fontAtlas.width;
			archive << fontAtlas.height;
			archive << fontAtlas.data;
		}).IsValid();
	}

	bool AssetDatabase::ImportSound(SoundImportSettings* settings) {
		return CreateAssetFile(settings->destPath.c_str(), AssetType::Sound, [&](SerializationArchive& archive) {
			std::vector<int8_t> soundData;
			FileSystem::ReadFile(settings->srcPath.c_str(), soundData);
			archive << soundData;
		}).IsValid();
	}

	bool AssetDatabase::ImportModel(ModelImportSettings* settings) {
		Model model(settings->srcPath.c_str());
		if (!model.IsValid()) {
			Log::Error("Failed to load model: %s", settings->srcPath.c_str());
			return false;
		}

		AssetHandle ret;

		bool isSkeletal = model.HasSkeleton();

		std::filesystem::path destPath = settings->destPath;
		std::string fileNamePrefix = destPath.stem().generic_string();

		std::unordered_map<Ref<Image>, AssetHandle> loadedImages;
		std::vector<AssetHandle> loadedMaterials;
		for (const auto& material : model.GetMaterials()) {
			MaterialCreateSettings matSet = {};
			matSet.destPath = destPath.replace_filename(fileNamePrefix + "_Material.asset").generic_string();
			matSet.shaderHandle = isSkeletal ? AssetManager::GetHandleByKey("std3d_geometry_skeletal") : AssetManager::GetHandleByKey("std3d_geometry_static");
			matSet.renderMode = RenderMode::Opaque;
			matSet.cullMode = CullMode::Back;
			matSet.depthTest = DepthTest::Less;
			matSet.depthWrite = true;

			std::vector<std::pair<std::string, Ref<Image>>> images = {
				{"Diffuse", material.diffuse},
				{"Normal", material.normal},
				{"Emissive", material.emissive},
				{"Metallic", material.metallic},
				{"Roughness", material.roughness},
				{"AmbientOcclusion", material.ambientOcclusion}
			};

			for (const auto& [kindStr, image] : images) {
				if (image) {
					if (loadedImages.find(image) == loadedImages.end()) {
						Texture2DCreateSettings textureSettings = {};
						textureSettings.destPath = destPath.replace_filename(fileNamePrefix + "_" + kindStr + ".asset").generic_string();
						textureSettings.format = PixelFormat::RGBA8;
						textureSettings.width = image->Width();
						textureSettings.height = image->Height();
						textureSettings.usageFlags = UsageFlag::Static;
						textureSettings.bindFlags = BindFlag::ShaderResource;
						textureSettings.accessFlags = 0;
						textureSettings.data = image->Data();

						loadedImages[image] = CreateAsset(&textureSettings);
					}

					if (kindStr == "Diffuse") {
						matSet.albedoTexture = loadedImages[image];
					}
					else if (kindStr == "Normal") {
						matSet.normalTexture = loadedImages[image];
					}
					else if (kindStr == "Emissive") {
						matSet.emissiveTexture = loadedImages[image];
					}
					else if (kindStr == "Metallic") {
						matSet.metallicTexture = loadedImages[image];
					}
					else if (kindStr == "Roughness") {
						matSet.roughnessTexture = loadedImages[image];
					}
					else if (kindStr == "AmbientOcclusion") {
						matSet.ambientOcclusionTexture = loadedImages[image];
					}
				}
			}

			loadedMaterials.push_back(CreateAsset(&matSet));
		}

		if (isSkeletal) {
			destPath.replace_filename(fileNamePrefix + "_SkeletalMesh.asset");

			ret = CreateAssetFile(destPath.generic_string().c_str(), AssetType::SkeletalMesh, [&](SerializationArchive& archive) {
				
				const ModelSkeleton& modelSkeleton = model.GetSkeleton();

				std::vector<MeshSegment> segments;
				std::vector<AssetHandle> materials;
				std::vector<Vertex3D> vertices;
				for (const auto& mesh : model.GetMeshs()) {
					segments.push_back(MeshSegment{ PrimitiveTopology::TriangleList, mesh.vertexStart, mesh.vertexCount, mesh.indexStart, mesh.indexCount });
					materials.push_back(loadedMaterials[mesh.materialIndex]);

					for (uint32_t i = 0; i < mesh.vertexCount; ++i) {
						const ModelVertex& vertex = model.GetVertexAt(mesh.vertexStart + i);
						const ModelVertexBoneData& vertexBoneData = model.GetVertexBoneDataAt(mesh.vertexStart + i);

						Vertex3D vertex3D = {};
						vertex3D.position = vertex.position;
						vertex3D.texcoord = vertex.texCoord;
						vertex3D.tangent = vertex.tangent;
						vertex3D.normal = vertex.normal;
						vertex3D.binormal = vertex.bitangent;
						for (int32_t j = 0; j < 4; ++j) {
							vertex3D.boneIndices[j] = vertexBoneData.boneIndices[j];
							vertex3D.boneWeights[j] = vertexBoneData.boneWeight[j];
						}

						vertices.push_back(vertex3D);
					}
				}

				SkeletonCreateSettings skeletonSettings = {};
				skeletonSettings.globalInvMatrix = model.GetGlobalInvMatrix();
				skeletonSettings.destPath = destPath.replace_filename(fileNamePrefix + "_Skeleton.asset").generic_string();
				std::transform(modelSkeleton.nodes.begin(), modelSkeleton.nodes.end(), std::back_inserter(skeletonSettings.nodes), [](const ModelSkeletonNode& node) {
					return SkeletonNode{ node.name, node.parentIndex, node.transformMatrix, node.childrenIndices };
				});
				std::transform(modelSkeleton.boneMap.begin(), modelSkeleton.boneMap.end(), std::inserter(skeletonSettings.boneMap, skeletonSettings.boneMap.end()), [](const auto& pair) {
					return std::make_pair(pair.first, SkeletonBoneNode{ pair.second.nodeIndex, pair.second.boneIndex, pair.second.offsetMatrix });
				});
				
				for (const auto& animation : model.GetSkeletalAnimations()) {
					SkeletalAnimationCreateSettings animSettings = {};
					animSettings.destPath = destPath.replace_filename(fileNamePrefix + "_" + animation.name + ".asset").generic_string();
					animSettings.name = animation.name;
					animSettings.durationSec = animation.durationSec;
					std::transform(animation._nodes.begin(), animation._nodes.end(), std::back_inserter(animSettings.animationNodes), [](const ModelSkeletalAnimationNode& boneAnim) {
						std::vector<SkeletalAnimationNodeKey<vec3>> positionKeys;
						std::transform(boneAnim.positionKeys.begin(), boneAnim.positionKeys.end(), std::back_inserter(positionKeys), [](const auto& key) { return SkeletalAnimationNodeKey<vec3>{key.first, key.second}; });

						std::vector<SkeletalAnimationNodeKey<vec4>> rotationKeys;
						std::transform(boneAnim.rotationKeys.begin(), boneAnim.rotationKeys.end(), std::back_inserter(rotationKeys), [](const auto& key) { return SkeletalAnimationNodeKey<vec4>{key.first, key.second}; });

						std::vector<SkeletalAnimationNodeKey<vec3>> scaleKeys;
						std::transform(boneAnim.scaleKeys.begin(), boneAnim.scaleKeys.end(), std::back_inserter(scaleKeys), [](const auto& key) { return SkeletalAnimationNodeKey<vec3>{key.first, key.second}; });
						
						return SkeletalAnimationNode(boneAnim.name, positionKeys, rotationKeys, scaleKeys);
					});
					skeletonSettings.animationHandles.push_back(CreateAsset(&animSettings));
				}

				archive << segments;
				archive << CreateAsset(&skeletonSettings);
				archive << materials;
				archive << vertices;
				archive << model.GetIndices();
			});
		}
		else {
			destPath.replace_filename(fileNamePrefix + "_StaticMesh.asset");

			ret = CreateAssetFile(destPath.generic_string().c_str(), AssetType::StaticMesh, [&](SerializationArchive& archive) {
				std::vector<MeshSegment> segments;
				std::vector<AssetHandle> materials;
				std::vector<Vertex3D> vertices;
				for (const auto& mesh : model.GetMeshs()) {
					segments.push_back(MeshSegment{ PrimitiveTopology::TriangleList, mesh.vertexStart, mesh.vertexCount, mesh.indexStart, mesh.indexCount });
					materials.push_back(loadedMaterials[mesh.materialIndex]);

					for (uint32_t i = 0; i < mesh.vertexCount; ++i) {
						const ModelVertex& vertex = model.GetVertexAt(mesh.vertexStart + i);
						const ModelVertexBoneData& vertexBoneData = model.GetVertexBoneDataAt(mesh.vertexStart + i);

						Vertex3D vertex3D = {};
						vertex3D.position = vertex.position;
						vertex3D.texcoord = vertex.texCoord;
						vertex3D.tangent = vertex.tangent;
						vertex3D.normal = vertex.normal;
						vertex3D.binormal = vertex.bitangent;
						for (int32_t j = 0; j < 4; ++j) {
							vertex3D.boneIndices[j] = vertexBoneData.boneIndices[j];
							vertex3D.boneWeights[j] = vertexBoneData.boneWeight[j];
						}

						vertices.push_back(vertex3D);
					}
				}

				archive << segments;
				archive << materials;
				archive << vertices;
				archive << model.GetIndices();
			});
		}

		return ret.Invalidate();
	}

	bool AssetDatabase::ImportGraphicsShader(const GraphicsShaderImportSettings* settings) {
		return CreateAssetFile(settings->destPath.c_str(), AssetType::GraphicsShader, [&](SerializationArchive& archive) {
			archive << settings->compileFlags;
			archive << settings->srcPath;
		}).IsValid();
	}
}