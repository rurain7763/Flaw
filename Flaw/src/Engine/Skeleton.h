#pragma once

#include "Core.h"
#include "Graphics.h"
#include "Utils/SerializationArchive.h"

namespace flaw {
	struct SkeletonBone {
		std::string name;
		int32_t parentIndex = -1;
		std::vector<int32_t> childrenIndices;
		mat4 offsetMatrix = mat4(1.0f);
		mat4 transformMatrix = mat4(1.0f);
	};

	template<>
	struct Serializer<SkeletonBone> {
		static void Serialize(SerializationArchive& archive, const SkeletonBone& value) {
			archive << value.name;
			archive << value.parentIndex;
			archive << value.childrenIndices;	
			archive << value.offsetMatrix;
			archive << value.transformMatrix;
		}

		static void Deserialize(SerializationArchive& archive, SkeletonBone& value) {
			archive >> value.name;
			archive >> value.parentIndex;
			archive >> value.childrenIndices;
			archive >> value.offsetMatrix;
			archive >> value.transformMatrix;
		}
	};

	class Skeleton {
	public:
		Skeleton(const mat4& globalInvMatrix, const std::unordered_map<std::string, int32_t>& boneNameMap, const std::vector<SkeletonBone>& bones);

		Ref<StructuredBuffer> GetBindingPosGPUBuffer() const { return _bindPosGPUBuffer; }

	private:
		void GenerateBindingPosMatrices();

	private:
		mat4 _globalInvMatrix = mat4(1.0f);
		std::unordered_map<std::string, int32_t> _boneNameMap;
		std::vector<SkeletonBone> _bones;

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
