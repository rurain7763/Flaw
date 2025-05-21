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
		enum class Type {
			Texture2D,
			Material,
			Skeleton,
			SkeletalAnimation,
		};

		Type type;
		std::string destPath;
	};

	struct Texture2DCreateSettings : public AssetCreateSettings {
		PixelFormat format;
		uint32_t width;
		uint32_t height;
		UsageFlag usageFlags;
		uint32_t bindFlags;
		uint32_t accessFlags;
		std::vector<uint8_t> data;
		
		Texture2DCreateSettings() {
			type = Type::Texture2D;
			format = PixelFormat::RGBA8;
			width = 0;
			height = 0;
			usageFlags = UsageFlag::Static;
			bindFlags = BindFlag::ShaderResource;
			accessFlags = 0;
			data.clear();
		}
	};

	struct MaterialCreateSettings : public AssetCreateSettings {
		RenderMode renderMode;
		CullMode cullMode;
		DepthTest depthTest;
		bool depthWrite;

		AssetHandle shaderHandle;
		AssetHandle albedoTexture;
		AssetHandle normalTexture;

		MaterialCreateSettings() {
			type = Type::Material;
		}
	};

	struct SkeletonCreateSettings : public AssetCreateSettings {
		mat4 globalInvMatrix = mat4(1.0f);
		std::vector<SkeletonNode> nodes;
		std::unordered_map<std::string, SkeletonBoneNode> boneMap;
		std::vector<AssetHandle> animationHandles;

		SkeletonCreateSettings() {
			type = Type::Skeleton;
		}
	};

	struct SkeletalAnimationCreateSettings : public AssetCreateSettings {
		std::string name;
		float durationSec = 0.0f;
		std::vector<SkeletalAnimationNode> animationNodes;

		SkeletalAnimationCreateSettings() {
			type = Type::SkeletalAnimation;
		}
	};

	struct AssetImportSettings {
		enum class Type {
			Texture2D,
			TextureCube,
			Texture2DArray,
			Font,
			Sound,
			Model,
			GraphicsShader,
		};

		Type type;
		std::string destPath;
	};

	struct Texture2DImportSettings : public AssetImportSettings {
		std::string srcPath;

		UsageFlag usageFlags;
		BindFlag bindFlags;
		uint32_t accessFlags;

		Texture2DImportSettings() {
			type = Type::Texture2D;
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
			type = Type::TextureCube;
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
			type = Type::Texture2DArray;
			usageFlags = UsageFlag::Static;
			bindFlags = BindFlag::ShaderResource;
			accessFlags = 0;
		}
	};

	struct FontImportSettings : public AssetImportSettings {
		std::string srcPath;

		FontImportSettings() {
			type = Type::Font;
		}
	};	

	struct SoundImportSettings : public AssetImportSettings {
		std::string srcPath;

		SoundImportSettings() {
			type = Type::Sound;
		}
	};

	struct ModelImportSettings : public AssetImportSettings {
		std::string srcPath;

		ModelImportSettings() {
			type = Type::Model;
		}
	};

	struct GraphicsShaderImportSettings : public AssetImportSettings {
		std::string srcPath;
		uint32_t compileFlags;

		GraphicsShaderImportSettings() {
			type = Type::GraphicsShader;
			compileFlags = 0;
		}
	};

	class AssetDatabase {
	public:
		static void Init(Application& application);
		static void Cleanup();

		static void Refresh(const char* folderPath, bool recursive);

		static bool GetAssetMetadata(const char* assetFile, AssetMetadata& outMetaData);

		static const std::filesystem::path& GetContentsDirectory();

		static AssetHandle CreateAsset(const AssetCreateSettings* settings);

		static bool ImportAsset(const AssetImportSettings* importSettings);

	private:
		static void RegisterAssetsInFolder(const char* folderPath, bool recursive = true);

		static Ref<Asset> CreateAssetInstance(AssetType assetType, const std::filesystem::path& path, int32_t dataOffset);

		static AssetHandle CreateAssetFile(const char* path, AssetType assetType, std::function<void(SerializationArchive&)> serializeFunc);

		static AssetHandle CreateTexture2D(const Texture2DCreateSettings* settings);
		static AssetHandle CreateMaterial(const MaterialCreateSettings* settings);
		static AssetHandle CreateSkeleton(const SkeletonCreateSettings* settings);
		static AssetHandle CreateSkeletalAnimation(const SkeletalAnimationCreateSettings* settings);

		static bool ImportTexture2D(Texture2DImportSettings* settings);
		static bool ImportTextureCube(TextureCubeImportSettings* settings);
		static bool ImportTexture2DArray(Texture2DArrayImportSettings* settings);
		static bool ImportFont(FontImportSettings* settings);
		static bool ImportSound(SoundImportSettings* settings);
		static bool ImportModel(ModelImportSettings* settings);
		static bool ImportGraphicsShader(const GraphicsShaderImportSettings* settings);
	};
}