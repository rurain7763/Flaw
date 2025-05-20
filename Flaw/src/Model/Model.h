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

namespace flaw {
	constexpr int32_t MaxBoneCount = 4;

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
		int32_t boneIndex[MaxBoneCount] = { -1, -1, -1, -1 };
		float boneWeight[MaxBoneCount] = { 0.0f, 0.0f, 0.0f, 0.0f };

		void AddBoneWeight(int32_t index, float weight) {
			for (int32_t i = 0; i < MaxBoneCount; ++i) {
				if (boneIndex[i] == -1) {
					boneIndex[i] = index;
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

	struct ModelMesh {
		uint32_t vertexStart = 0;
		uint32_t vertexCount = 0;
		uint32_t indexStart = 0;
		uint32_t indexCount = 0;
		uint32_t materialIndex = 0;
	};

	struct ModelSkeleton {
		uint32_t boneStart = 0;
		uint32_t boneCount = 0;
	};

	struct ModelBone {
		std::string name;
		mat4 offsetMatrix;
		mat4 transform;
		int32_t parentIndex = -1;
	};
	
	class Model {
	public:
		Model() = default;
		Model(const char* filePath);
		Model(ModelType type, const char* basePath, const char* memory, size_t size);

		ModelType GetModelType() const { return _type; }

		const uint32_t GetMaterialCount() const { return _materials.size(); }
		const ModelMaterial& GetMaterialAt(uint32_t index) const { return _materials[index]; }
		const std::vector<ModelVertex>& GetVertices() const { return _vertices; }
		const std::vector<uint32_t>& GetIndices() const { return _indices; }
		const std::vector<ModelMesh>& GetMeshs() const { return _meshes; }
		const std::vector<ModelSkeleton>& GetSkeletons() const { return _skeletons; }
		const std::vector<ModelBone>& GetBones() const { return _bones; }

		bool HasSkeleton() const { return !_skeletons.empty(); }

		bool IsValid() const { return !_meshes.empty(); }

	private:
		void ParseScene(std::filesystem::path basePath, const aiScene* scene);
		void ParseMaterial(const std::filesystem::path& basePath, const aiMaterial* aiMaterial, ModelMaterial& material);
		void ParseMesh(const aiScene* scene, const aiMesh* mesh, ModelMesh& modelMesh);
		void ParseBones(const aiScene* scene, const aiMesh* mesh, ModelMesh& modelMesh);
		void SetBonesTransform(const std::map<std::string, int32_t>& boneMap, const aiNode* node, const mat4& parentTransform, const mat4& globalInv);

		Ref<Image> GetImageOrCreate(const std::filesystem::path& path);

	private: 
		ModelType _type = ModelType::Unknown;

		std::unordered_map<std::filesystem::path, Ref<Image>> _images;
		std::vector<ModelMaterial> _materials;

		std::vector<ModelVertex> _vertices;
		std::vector<uint32_t> _indices;

		std::vector<ModelBone> _bones;

		std::vector<ModelMesh> _meshes;
		std::vector<ModelSkeleton> _skeletons;
	};
}