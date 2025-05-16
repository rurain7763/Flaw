#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Image/Image.h"

struct aiScene;
struct aiMesh;
struct aiMaterial;
struct aiTexture;

namespace flaw {
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

		bool HasSkeleton() const { return true; }

		bool IsValid() const { return !_meshes.empty(); }

	private:
		void ParseScene(std::filesystem::path basePath, const aiScene* scene);
		void ParseMaterial(const std::filesystem::path& basePath, const aiMaterial* aiMaterial, ModelMaterial& material);
		void ParseMesh(const aiMesh* mesh, ModelMesh& meshInfo);

		Ref<Image> GetImageOrCreate(const std::filesystem::path& path);

	private: 
		ModelType _type = ModelType::Unknown;

		std::unordered_map<std::filesystem::path, Ref<Image>> _images;
		std::vector<ModelMaterial> _materials;

		std::vector<ModelVertex> _vertices;
		std::vector<uint32_t> _indices;

		std::vector<ModelMesh> _meshes;
	};
}