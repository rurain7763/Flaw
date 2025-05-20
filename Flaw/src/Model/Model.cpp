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
			ParseMesh(scene, mesh, _meshes[i]);
		}

		for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
			const aiAnimation* animation = scene->mAnimations[i];

			// TODO: 애니메이션 파싱
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

		if (aiMaterial->GetTexture(aiTextureType_HEIGHT, 0, &path) == AI_SUCCESS) {
			// TODO: height map
		}

		if (aiMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &path) == AI_SUCCESS) {
			material.diffuse = GetImageOrCreate(basePath / path.C_Str());
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

	void Model::ParseMesh(const aiScene* scene, const aiMesh* mesh, ModelMesh& modelMesh) {
		modelMesh.vertexStart = static_cast<uint32_t>(_vertices.size());
		modelMesh.vertexCount = mesh->mNumVertices;
		modelMesh.indexStart = static_cast<uint32_t>(_indices.size());
		modelMesh.indexCount = mesh->mNumFaces * 3;
		modelMesh.materialIndex = mesh->mMaterialIndex;

		_vertices.reserve(_vertices.size() + mesh->mNumVertices);
		for (int32_t i = 0; i < mesh->mNumVertices; ++i) {
			ModelVertex vertex;
			vertex.position = vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			vertex.normal = mesh->HasNormals() ? vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : vec3(0.0f);
			vertex.texCoord = mesh->HasTextureCoords(0) ? vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : vec2(0.0f);
			vertex.tangent = mesh->HasTangentsAndBitangents() ? vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z) : vec3(0.0f);
			vertex.bitangent = mesh->HasTangentsAndBitangents() ? vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z) : vec3(0.0f);
			
			_vertices.push_back(vertex);
		}

		_indices.reserve(_indices.size() + mesh->mNumFaces * 3);
		for (int32_t i = 0; i < mesh->mNumFaces; ++i) {
			const aiFace& face = mesh->mFaces[i];
			for (int32_t j = 0; j < face.mNumIndices; ++j) {
				_indices.push_back(face.mIndices[j]);
			}
		}

		ParseBones(scene, mesh, modelMesh);
	}

	mat4 ToMat4(const aiMatrix4x4& mat) {
		return mat4(
			mat.a1, mat.b1, mat.c1, mat.d1,
			mat.a2, mat.b2, mat.c2, mat.d2,
			mat.a3, mat.b3, mat.c3, mat.d3,
			mat.a4, mat.b4, mat.c4, mat.d4
		);
	}

	void Model::ParseBones(const aiScene* scene, const aiMesh* mesh, ModelMesh& modelMesh) {
		if (mesh->HasBones()) {
			ModelSkeleton skeleton;
			skeleton.boneStart = static_cast<uint32_t>(_bones.size());
			skeleton.boneCount = mesh->mNumBones;

			std::map<std::string, int32_t> boneMap;

			_bones.resize(_bones.size() + mesh->mNumBones);
			for (int32_t i = 0; i < mesh->mNumBones; ++i) {
				const aiBone* bone = mesh->mBones[i];
				ModelBone& modelBone = _bones[skeleton.boneStart + i];

				modelBone.name = bone->mName.C_Str();
				modelBone.offsetMatrix = ToMat4(bone->mOffsetMatrix);

				for (int32_t j = 0; j < bone->mNumWeights; ++j) {
					const aiVertexWeight& weight = bone->mWeights[j];
					_vertices[modelMesh.vertexStart + weight.mVertexId].AddBoneWeight(i, weight.mWeight);
				}

				boneMap[modelBone.name] = skeleton.boneStart + i;
			}

			mat4 globalInverse = glm::inverse(ToMat4(scene->mRootNode->mTransformation));
			SetBonesTransform(boneMap, scene->mRootNode, mat4(1.0f), globalInverse);

			_skeletons.push_back(skeleton);
		}
	}

	void Model::SetBonesTransform(const std::map<std::string, int32_t>& boneMap, const aiNode* node, const mat4& parentTransform, const mat4& globalInv) {
		mat4 finalTransform = parentTransform * ToMat4(node->mTransformation);
		auto it = boneMap.find(node->mName.C_Str());
		if (it != boneMap.end()) {
			ModelBone& bone = _bones[it->second];
			bone.transform = globalInv * finalTransform * bone.offsetMatrix;
		}
		for (int32_t i = 0; i < node->mNumChildren; ++i) {
			SetBonesTransform(boneMap, node->mChildren[i], finalTransform, globalInv);
		}
	}
}