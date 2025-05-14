#include "pch.h"
#include "Model.h"
#include "Log/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace flaw {
	constexpr uint32_t LoadOptFlags = 
		aiProcess_Triangulate | 
		aiProcess_FlipUVs | 
		aiProcess_MakeLeftHanded |
		aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals |
		aiProcess_JoinIdenticalVertices;

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

		ParseScene(scene);
	}

	Model::Model(ModelType type, const char* memory, size_t size) {
		_type = type;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFileFromMemory(memory, size, LoadOptFlags);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			Log::Error("ASSIMP: %s", importer.GetErrorString());
			return;
		}

		ParseScene(scene);
	}

	void Model::ParseScene(const aiScene* scene) {
		for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
			const aiMesh* mesh = scene->mMeshes[i];

			ParseMesh(mesh);
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

	void Model::ParseMesh(const aiMesh* mesh) {
		ModelMeshInfo meshInfo;
		meshInfo.vertexStart = static_cast<uint32_t>(_vertices.size());
		meshInfo.vertexCount = mesh->mNumVertices;
		meshInfo.indexStart = static_cast<uint32_t>(_indices.size());
		meshInfo.indexCount = mesh->mNumFaces * 3;

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
				_indices.push_back(meshInfo.vertexStart + face.mIndices[j]);
			}
		}

		_meshes.push_back(meshInfo);
	}
}