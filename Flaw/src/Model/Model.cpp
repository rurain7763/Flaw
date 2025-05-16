#include "pch.h"
#include "Model.h"
#include "Log/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace flaw {
	constexpr uint32_t LoadOptFlags =
		aiProcess_Triangulate |
		aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ConvertToLeftHanded;

	Model::Model(const char* filePath) {
		std::string extension = std::filesystem::path(filePath).extension().generic_string();
		
		_type = ModelType::Unknown;
		if (extension == ".obj") {
			_type = ModelType::Obj;
		}
		else if (extension == ".fbx") {
			_type = ModelType::Fbx;
		}
		
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filePath, LoadOptFlags);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			Log::Error("ASSIMP: %s", importer.GetErrorString());
			return;
		}

		ParseScene(std::filesystem::path(filePath).parent_path(), scene);
	}

	Model::Model(ModelType type, const char* basePath, const char* memory, size_t size) {
		_type = type;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFileFromMemory(memory, size, LoadOptFlags);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			Log::Error("ASSIMP: %s", importer.GetErrorString());
			return;
		}

		ParseScene(std::filesystem::path(basePath), scene);
	}

	void Model::ParseScene(std::filesystem::path basePath, const aiScene* scene) {
		_materials.resize(scene->mNumMaterials);
		for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
			const aiMaterial* material = scene->mMaterials[i];
			ParseMaterial(basePath, material, _materials[i]);
		}

		_meshes.resize(scene->mNumMeshes);
		for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
			const aiMesh* mesh = scene->mMeshes[i];
			ParseMesh(mesh, _meshes[i]);
		}

		for (uint32_t i = 0; i < scene->mNumSkeletons; ++i) {
			const aiSkeleton* skeleton = scene->mSkeletons[i];

			// TODO: ½ºÄÌ·¹Åæ ÆÄ½Ì
		}

		for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
			const aiAnimation* animation = scene->mAnimations[i];

			// TODO: ¾Ö´Ï¸ÞÀÌ¼Ç ÆÄ½Ì
		}
	}

	void Model::ParseMaterial(const std::filesystem::path& basePath, const aiMaterial* aiMaterial, ModelMaterial& material) {
		aiString path;
		if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
			material.diffuse = GetImageOrCreate(basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
			material.normal = GetImageOrCreate(basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_SPECULAR, 0, &path) == AI_SUCCESS) {
			material.specular = GetImageOrCreate(basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &path) == AI_SUCCESS) {
			material.emissive = GetImageOrCreate(basePath / path.C_Str());
		}

		// TODO: add more texture
	}

	Ref<Image> Model::GetImageOrCreate(const std::filesystem::path& path) {
		auto it = _images.find(path);
		if (it != _images.end()) {
			return it->second;
		}

		Ref<Image> image = CreateRef<Image>(path.generic_string().c_str(), 4);
		if (image->IsValid()) {
			_images[path] = image;
			return image;
		}

		return nullptr;
	}

	void Model::ParseMesh(const aiMesh* mesh, ModelMesh& meshInfo) {
		meshInfo.vertexStart = static_cast<uint32_t>(_vertices.size());
		meshInfo.vertexCount = mesh->mNumVertices;
		meshInfo.indexStart = static_cast<uint32_t>(_indices.size());
		meshInfo.indexCount = mesh->mNumFaces * 3;
		meshInfo.materialIndex = mesh->mMaterialIndex;

		for (int32_t i = 0; i < mesh->mNumVertices; ++i) {
			ModelVertex vertex;
			vertex.position = vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			vertex.normal = mesh->HasNormals() ? vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : vec3(0.0f);
			vertex.texCoord = mesh->HasTextureCoords(0) ? vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : vec2(0.0f);
			vertex.tangent = mesh->HasTangentsAndBitangents() ? vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z) : vec3(0.0f);
			vertex.bitangent = mesh->HasTangentsAndBitangents() ? vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z) : vec3(0.0f);

			_vertices.push_back(vertex);
		}

		for (int32_t i = 0; i < mesh->mNumFaces; ++i) {
			const aiFace& face = mesh->mFaces[i];
			for (int32_t j = 0; j < face.mNumIndices; ++j) {
				_indices.push_back(face.mIndices[j]);
			}
		}
	}
}