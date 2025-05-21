#pragma once

#include "Core.h"
#include "Graphics.h"
#include "Utils/SerializationArchive.h"

namespace flaw {
	struct SkeletonNode {
		std::string name;
		int32_t parentIndex = -1;
		mat4 transformMatrix = mat4(1.0f);
		std::vector<int32_t> childrenIndices;
	};

	template<>
	struct Serializer<SkeletonNode> {
		static void Serialize(SerializationArchive& archive, const SkeletonNode& value) {
			archive << value.name;
			archive << value.parentIndex;
			archive << value.transformMatrix;
			archive << value.childrenIndices;
		}

		static void Deserialize(SerializationArchive& archive, SkeletonNode& value) {
			archive >> value.name;
			archive >> value.parentIndex;
			archive >> value.transformMatrix;
			archive >> value.childrenIndices;
		}
	};

	struct SkeletonBoneNode {
		int32_t nodeIndex = -1;
		mat4 offsetMatrix = mat4(1.0f);
	};

	template<>
	struct Serializer<SkeletonBoneNode> {
		static void Serialize(SerializationArchive& archive, const SkeletonBoneNode& value) {
			archive << value.nodeIndex;
			archive << value.offsetMatrix;
		}

		static void Deserialize(SerializationArchive& archive, SkeletonBoneNode& value) {
			archive >> value.nodeIndex;
			archive >> value.offsetMatrix;
		}
	};

	class Skeleton {
	public:
		Skeleton(const mat4& globalInvMatrix, const std::vector<SkeletonNode>& nodes, const std::unordered_map<std::string, SkeletonBoneNode>& boneMap);

		const mat4& GetGlobalInvMatrix() const { return _globalInvMatrix; }
		const std::vector<SkeletonNode>& GetNodes() const { return _nodes; }
		const std::unordered_map<std::string, SkeletonBoneNode>& GetBoneMap() const { return _boneMap; }

		Ref<StructuredBuffer> GetBindingPosGPUBuffer() const { return _bindPosGPUBuffer; }

	private:
		void GenerateBindingPosMatrices();

	private:
		mat4 _globalInvMatrix = mat4(1.0f);
		std::vector<SkeletonNode> _nodes;
		std::unordered_map<std::string, SkeletonBoneNode> _boneMap;

		Ref<StructuredBuffer> _bindPosGPUBuffer;
	};

	struct SkeletalAnimationBone {
		std::string boneName;

		std::vector<std::pair<float, vec3>> positionKeys;
		std::vector<std::pair<float, vec4>> rotationKeys;
		std::vector<std::pair<float, vec3>> scaleKeys;
	};

	class SkeletalAnimation {
	public:

	};
}
