#pragma once

#include "Asset.h"
#include "Graphics.h"
#include "Math/Math.h"
#include "Utils/Raycast.h"
#include "Graphics/Texture.h"
#include "Font/Font.h"
#include "Sound/SoundSource.h"
#include "Graphics/GraphicsBuffers.h"
#include "Mesh.h"
#include "Material.h"
#include "Skeleton.h"
#include "Prefab.h"

namespace flaw {
	class Texture2DAsset : public Asset {
	public:
		struct Descriptor {
			PixelFormat format;
			uint32_t width;
			uint32_t height;
			UsageFlag usage;
			BindFlag bindFlags;
			uint32_t accessFlags;
			std::vector<uint8_t> data;
		};

		Texture2DAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Texture2D; }
		bool IsLoaded() const override { return _texture != nullptr; }

		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		const Ref<Texture2D>& GetTexture() const { return _texture; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<Texture2D> _texture;
	};

	class Texture2DArrayAsset : public Asset {
	public:
		struct Descriptor {
			uint32_t arraySize;
			PixelFormat format;
			uint32_t width;
			uint32_t height;
			UsageFlag usage;
			uint32_t accessFlags;
			BindFlag bindFlags;
			std::vector<uint8_t> data;
		};

		Texture2DArrayAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Texture2DArray; }
		bool IsLoaded() const override { return _texture != nullptr; }

		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		const Ref<Texture2DArray>& GetTexture() const { return _texture; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<Texture2DArray> _texture;
	};

	class TextureCubeAsset : public Asset {
	public:
		struct Descriptor {
			PixelFormat format;
			uint32_t width;
			uint32_t height;
			TextureCube::Layout layout;
			std::vector<uint8_t> data;
		};

		TextureCubeAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::TextureCube; }
		bool IsLoaded() const override { return _texture != nullptr; }

		const Ref<TextureCube>& GetTexture() const { return _texture; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<TextureCube> _texture;
	};

	class FontAsset : public Asset {
	public:
		struct Descriptor {
			std::vector<int8_t> fontData;
			uint32_t width;
			uint32_t height;
			std::vector<uint8_t> atlasData;
		};

		FontAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Font; }
		bool IsLoaded() const override { return _font != nullptr; }
		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		const Ref<Font>& GetFont() const { return _font; }
		const Ref<Texture2D>& GetFontAtlas() const { return _fontAtlas; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<Font> _font;
		Ref<Texture2D> _fontAtlas;
	};

	class SoundAsset : public Asset {
	public:
		struct Descriptor {
			std::vector<int8_t> soundData;
		};

		SoundAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Sound; }
		bool IsLoaded() const override { return _sound != nullptr; }
		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		const Ref<SoundSource>& GetSoundSource() const { return _sound; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<SoundSource> _sound;
	};

	class StaticMeshAsset : public Asset {
	public:
		struct Descriptor {
			std::vector<MeshSegment> segments;
			std::vector<AssetHandle> materials;
			std::vector<Vertex3D> vertices;
			std::vector<uint32_t> indices;
		};

		StaticMeshAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::StaticMesh; }
		bool IsLoaded() const override { return _mesh != nullptr; }

		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		const Ref<Mesh>& GetMesh() const { return _mesh; }
		const std::vector<AssetHandle>& GetMaterialHandles() const { return _materials; }
	
	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<Mesh> _mesh;
		std::vector<AssetHandle> _materials;
	};

	class SkeletalMeshAsset : public Asset {
	public:
		struct Descriptor {
			std::vector<MeshSegment> segments;
			AssetHandle skeleton;
			std::vector<AssetHandle> materials;
			std::vector<SkinnedVertex3D> vertices;
			std::vector<uint32_t> indices;
		};

		SkeletalMeshAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::SkeletalMesh; }
		bool IsLoaded() const override { return _mesh != nullptr; }

		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		Ref<Mesh> GetMesh() const { return _mesh; }

		const std::vector<AssetHandle>& GetMaterialHandles() const { return _materials; }
		const AssetHandle& GetMaterialHandleAt(uint32_t index) const { return _materials[index]; }
		const AssetHandle& GetSkeletonHandle() const { return _skeleton; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<Mesh> _mesh;
		std::vector<AssetHandle> _materials;
		AssetHandle _skeleton;
	};

	class GraphicsShaderAsset : public Asset {
	public:
		struct Descriptor {
			uint32_t shaderCompileFlags;

			// TODO: this should be change to actually code source not path
			std::string shaderPath;
			std::vector<GraphicsShader::InputElement> inputElements;
		};

		GraphicsShaderAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::GraphicsShader; }
		bool IsLoaded() const override { return _shader != nullptr; }

		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		const Ref<GraphicsShader>& GetShader() const { return _shader; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<GraphicsShader> _shader;
	};

	class MaterialAsset : public Asset {
	public:
		struct Descriptor {
			RenderMode renderMode;
			CullMode cullMode;
			DepthTest depthTest;
			bool depthWrite;

			AssetHandle shaderHandle;
			AssetHandle albedoTexture;
			AssetHandle normalTexture;
			AssetHandle emissiveTexture;
			AssetHandle metallicTexture;
			AssetHandle roughnessTexture;
			AssetHandle ambientOcclusionTexture;
		};

		MaterialAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Material; }
		bool IsLoaded() const override { return _material != nullptr; }

		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		const Ref<Material>& GetMaterial() const { return _material; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<Material> _material;
	};

	class SkeletonAsset : public Asset {
	public:
		struct Descriptor {
			mat4 globalInvMatrix;
			std::vector<SkeletonNode> nodes;
			std::unordered_map<std::string, SkeletonBoneNode> boneMap;
			std::vector<AssetHandle> animationHandles;
		};

		SkeletonAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Skeleton; }
		bool IsLoaded() const override { return _skeleton != nullptr; }

		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		const Ref<Skeleton>& GetSkeleton() const { return _skeleton; }
		const std::vector<AssetHandle>& GetAnimationHandles() const { return _animationHandles; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<Skeleton> _skeleton;
		std::vector<AssetHandle> _animationHandles;
	};

	class SkeletalAnimationAsset : public Asset {
	public:
		struct Descriptor {
			std::string name;
			float durationSec;
			std::vector<SkeletalAnimationNode> animationNodes;
		};

		SkeletalAnimationAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::SkeletalAnimation; }
		bool IsLoaded() const override { return _animation != nullptr; }

		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		const Ref<SkeletalAnimation>& GetAnimation() const { return _animation; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<SkeletalAnimation> _animation;
	};

	class PrefabAsset : public Asset {
	public:
		struct Descriptor {
			std::vector<int8_t> prefabData; // Serialized data of the prefab
		};

		PrefabAsset(const std::function<void(Descriptor&)>& getDesc) : _getDesc(getDesc) {}

		void Load() override;
		void Unload() override;

		AssetType GetAssetType() const override { return AssetType::Prefab; }
		bool IsLoaded() const override { return _prefab != nullptr; }

		void GetDescriptor(Descriptor& desc) const { _getDesc(desc); }

		const Ref<Prefab>& GetPrefab() const { return _prefab; }

	private:
		std::function<void(Descriptor&)> _getDesc;

		Ref<Prefab> _prefab;
	};
}