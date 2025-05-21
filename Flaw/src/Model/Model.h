#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Image/Image.h"

#include <map>

struct aiScene;
struct aiMesh;
struct aiMaterial;
struct aiTexture;
struct aiSkeleton;
struct aiBone;
struct aiNode;
struct aiAnimation;
struct aiNodeAnim;

namespace flaw {
	constexpr int32_t MaxInfluenceBoneCount = 4;

	enum class ModelType {
		Obj,
		Fbx,
		Unknown
	};

	struct ModelVertex {
		vec3 position;
		vec3 normal;
		vec2 texCoord;
		vec3 tangent;
		vec3 bitangent;
	};

	struct ModelVertexBoneData {
		std::string boneNames[MaxInfluenceBoneCount] = { "", "", "", ""};
		float boneWeight[MaxInfluenceBoneCount] = { 0.0f, 0.0f, 0.0f, 0.0f };

		void AddBoneWeight(const char* boneName, float weight) {
			for (int32_t i = 0; i < MaxInfluenceBoneCount; ++i) {
				if (boneNames[i] == "") {
					boneNames[i] = boneName;
					boneWeight[i] = weight;
					return;
				}
			}
		}
	};

	struct ModelMaterial {
		Ref<Image> diffuse;
		Ref<Image> normal;
		Ref<Image> specular;
		Ref<Image> emissive;
	};

	struct ModelSkeletonNode {
		std::string name;
		int32_t parentIndex = -1;
		mat4 transformMatrix = mat4(1.0f);
		std::vector<int32_t> childrenIndices;
	};

	struct ModelSkeletonBoneNode {
		int32_t nodeIndex = -1;
		mat4 offsetMatrix = mat4(1.0f);
	};

	struct ModelSkeleton {
		std::vector<ModelSkeletonNode> nodes;
		std::unordered_map<std::string, ModelSkeletonBoneNode> boneMap;

		int32_t FindNode(const std::string& name) const {
			for (size_t i = 0; i < nodes.size(); ++i) {
				if (nodes[i].name == name) {
					return static_cast<int32_t>(i);
				}
			}
			return -1;
		}
	};

	struct ModelBoneAnimation {
		std::string boneName;

		std::vector<std::pair<float, vec3>> positionKeys;
		std::vector<std::pair<float, vec4>> rotationKeys;
		std::vector<std::pair<float, vec3>> scaleKeys;
	};
	
	struct ModelSkeletalAnimation {
		std::string name;
		float duration = 0.0f;
		float ticksPerSecond = 0.0f;

		std::vector<ModelBoneAnimation> boneAnimations;
	};

	struct ModelMesh {
		uint32_t vertexStart = 0;
		uint32_t vertexCount = 0;
		uint32_t indexStart = 0;
		uint32_t indexCount = 0;
		int32_t materialIndex = -1;
	};

	class Model {
	public:
		Model() = default;
		Model(const char* filePath);
		Model(ModelType type, const char* basePath, const char* memory, size_t size);

		ModelType GetModelType() const { return _type; }

		const std::vector<ModelMaterial>& GetMaterials() const { return _materials; }
		const ModelMaterial& GetMaterialAt(uint32_t index) const { return _materials[index]; }
		const ModelSkeleton& GetSkeleton() const { return _skeleton; }
		const std::vector<ModelVertex>& GetVertices() const { return _vertices; }
		const ModelVertex& GetVertexAt(uint32_t index) const { return _vertices[index]; }
		const ModelVertexBoneData& GetVertexBoneDataAt(uint32_t index) const { return _vertexBoneData[index]; }
		const std::vector<uint32_t>& GetIndices() const { return _indices; }
		const std::vector<ModelMesh>& GetMeshs() const { return _meshes; }

		const mat4& GetGlobalInvMatrix() const { return _globalInvMatrix; }

		bool HasSkeleton() const { return !_skeleton.boneMap.empty(); }

		bool IsValid() const { return !_meshes.empty(); }

	private:
		void ParseScene(std::filesystem::path basePath, const aiScene* scene);
		void ParseMaterial(const std::filesystem::path& basePath, const aiMaterial* aiMaterial, ModelMaterial& material);
		void ParseSkeleton(const aiScene* scene);
		void ParseMesh(const aiScene* scene, const aiMesh* mesh, ModelMesh& modelMesh);
		void ParseBones(const aiScene* scene, const aiMesh* mesh, ModelMesh& modelMesh);
		void ParseAnimation(const aiScene* scene, const aiAnimation* animation, ModelSkeletalAnimation& skeletalAnim);

		Ref<Image> GetImageOrCreate(const std::filesystem::path& path);

	private: 
		ModelType _type = ModelType::Unknown;

		mat4 _globalInvMatrix = mat4(1.0f);

		std::unordered_map<std::filesystem::path, Ref<Image>> _images;
		std::vector<ModelMaterial> _materials;

		std::vector<ModelVertex> _vertices;
		std::vector<ModelVertexBoneData> _vertexBoneData;
		std::vector<uint32_t> _indices;

		ModelSkeleton _skeleton;
		std::vector<ModelSkeletalAnimation> _skeletalAnimations;

		std::vector<ModelMesh> _meshes;
	};
}