#pragma once

#include "Core.h"
#include "Math/Math.h"

struct aiScene;
struct aiMesh;

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

	struct ModelMeshInfo {
		uint32_t vertexStart = 0;
		uint32_t vertexCount = 0;
		uint32_t indexStart = 0;
		uint32_t indexCount = 0;
	};
	
	class Model {
	public:
		Model() = default;
		Model(const char* filePath);
		Model(ModelType type, const char* memory, size_t size);

		ModelType GetModelType() const { return _type; }

		int32_t GetMeshCount() const { return static_cast<int32_t>(_meshes.size()); }
		const std::vector<ModelVertex>& GetVertices() const { return _vertices; }
		const std::vector<uint32_t>& GetIndices() const { return _indices; }
		const std::vector<ModelMeshInfo>& GetMeshInfos() const { return _meshes; }

		bool HasSkeleton() const { return true; }

		bool IsValid() const { return !_meshes.empty(); }

	private:
		void ParseScene(const aiScene* scene);
		void ParseMesh(const aiMesh* mesh);

	private: 
		ModelType _type = ModelType::Unknown;

		std::vector<ModelVertex> _vertices;
		std::vector<uint32_t> _indices;

		std::vector<ModelMeshInfo> _meshes;
	};
}