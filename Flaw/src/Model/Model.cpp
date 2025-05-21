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

	mat4 ToMat4(const aiMatrix4x4& mat) {
		return mat4(
			mat.a1, mat.b1, mat.c1, mat.d1,
			mat.a2, mat.b2, mat.c2, mat.d2,
			mat.a3, mat.b3, mat.c3, mat.d3,
			mat.a4, mat.b4, mat.c4, mat.d4
		);
	}

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
		_globalInvMatrix = ToMat4(scene->mRootNode->mTransformation);

		ParseSkeleton(scene);

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

		_skeletalAnimations.resize(scene->mNumAnimations);
		for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
			const aiAnimation* animation = scene->mAnimations[i];
			ParseAnimation(scene, animation, _skeletalAnimations[i]);
		}
	}

	void BuildSkeleton(ModelSkeleton& result, const aiNode* current, int32_t parentIndex) {
		int32_t nodeIndex = result.nodes.size();
		std::string nodeName = current->mName.C_Str();

		ModelSkeletonNode node;
		node.name = nodeName;
		node.parentIndex = parentIndex;
		node.transformMatrix = ToMat4(current->mTransformation);

		result.nodes.emplace_back(node);
		if (parentIndex != -1) {
			result.nodes[parentIndex].childrenIndices.push_back(nodeIndex);
		}

		for (uint32_t i = 0; i < current->mNumChildren; ++i) {
			BuildSkeleton(result, current->mChildren[i], nodeIndex);
		}
	}

	void Model::ParseSkeleton(const aiScene* scene) {
		BuildSkeleton(_skeleton, scene->mRootNode, -1);
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

		if (mesh->HasBones()) {
			ParseBones(scene, mesh, modelMesh);
		}
	}

	void Model::ParseBones(const aiScene* scene, const aiMesh* mesh, ModelMesh& modelMesh) {
		_vertexBoneData.resize(_vertexBoneData.size() + modelMesh.vertexCount);
		for (int32_t i = 0; i < mesh->mNumBones; ++i) {
			const aiBone* bone = mesh->mBones[i];
			const char* boneName = bone->mName.C_Str();

			for (int32_t j = 0; j < bone->mNumWeights; ++j) {
				const aiVertexWeight& weight = bone->mWeights[j];

				ModelVertexBoneData& vertexBoneData = _vertexBoneData[modelMesh.vertexStart + weight.mVertexId];
				vertexBoneData.AddBoneWeight(boneName, weight.mWeight);
			}

			auto it = _skeleton.boneMap.find(boneName);
			if (it == _skeleton.boneMap.end()) {
				_skeleton.boneMap[boneName] = ModelSkeletonBoneNode{ _skeleton.FindNode(boneName), ToMat4(bone->mOffsetMatrix) };
			}
		}
	}

	void Model::ParseAnimation(const aiScene* scene, const aiAnimation* animation, ModelSkeletalAnimation& skeletalAnim) {
		skeletalAnim.name = animation->mName.C_Str();
		skeletalAnim.duration = animation->mDuration;
		skeletalAnim.ticksPerSecond = animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f;

		skeletalAnim.boneAnimations.resize(animation->mNumChannels);
		for (int32_t i = 0; i < animation->mNumChannels; ++i) {
			const aiNodeAnim* channel = animation->mChannels[i];

			ModelBoneAnimation& boneAnim = skeletalAnim.boneAnimations[i];
			boneAnim.boneName = channel->mNodeName.C_Str();
			
			boneAnim.positionKeys.resize(channel->mNumPositionKeys);
			for (int32_t j = 0; j < channel->mNumPositionKeys; ++j) {
				boneAnim.positionKeys[j] = {
					channel->mPositionKeys[j].mTime / skeletalAnim.ticksPerSecond,
					vec3(channel->mPositionKeys[j].mValue.x, channel->mPositionKeys[j].mValue.y, channel->mPositionKeys[j].mValue.z)
				};
			}

			boneAnim.rotationKeys.resize(channel->mNumRotationKeys);
			for (int32_t j = 0; j < channel->mNumRotationKeys; ++j) {
				boneAnim.rotationKeys[j] = {
					channel->mRotationKeys[j].mTime / skeletalAnim.ticksPerSecond,
					vec4(channel->mRotationKeys[j].mValue.x, channel->mRotationKeys[j].mValue.y, channel->mRotationKeys[j].mValue.z, channel->mRotationKeys[j].mValue.w)
				};
			}

			boneAnim.scaleKeys.resize(channel->mNumScalingKeys);
			for (int32_t j = 0; j < channel->mNumScalingKeys; ++j) {
				boneAnim.scaleKeys[j] = {
					channel->mScalingKeys[j].mTime / skeletalAnim.ticksPerSecond,
					vec3(channel->mScalingKeys[j].mValue.x, channel->mScalingKeys[j].mValue.y, channel->mScalingKeys[j].mValue.z)
				};
			}
		}
	}
}