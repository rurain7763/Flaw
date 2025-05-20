#pragma once

#include "Core.h"
#include "Graphics.h"
#include "Utils/SerializationArchive.h"

namespace flaw {
	struct SkeletonSegment {
		uint32_t boneStart = 0;
		uint32_t boneCount = 0;
	};

	template<>
	struct Serializer<SkeletonSegment> {
		static void Serialize(SerializationArchive& archive, const SkeletonSegment& value) {
			archive << value.boneStart;
			archive << value.boneCount;
		}
		static void Deserialize(SerializationArchive& archive, SkeletonSegment& value) {
			archive >> value.boneStart;
			archive >> value.boneCount;
		}
	};

	struct SkeletonBoneMetadata {
		std::string name;
	};

	template<>
	struct Serializer<SkeletonBoneMetadata> {
		static void Serialize(SerializationArchive& archive, const SkeletonBoneMetadata& value) {
			archive << value.name;
		}

		static void Deserialize(SerializationArchive& archive, SkeletonBoneMetadata& value) {
			archive >> value.name;
		}
	};

	struct SkeletonBone {
		mat4 transformMatrix;
	};

	template<>
	struct Serializer<SkeletonBone> {
		static void Serialize(SerializationArchive& archive, const SkeletonBone& value) {
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					archive << value.transformMatrix[i][j];
				}
			}
		}

		static void Deserialize(SerializationArchive& archive, SkeletonBone& value) {
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					archive >> value.transformMatrix[i][j];
				}
			}
		}
	};

	class Skeleton {
	public:
		Skeleton() = default;
		Skeleton(const std::vector<SkeletonBoneMetadata>& boneMetas, const std::vector<SkeletonBone>& bones);
		Skeleton(const std::vector<SkeletonBoneMetadata>& boneMetas, const std::vector<SkeletonBone>& bones, const std::vector<SkeletonSegment>& segments);

		void GetBindPosMatrices(std::vector<mat4>& out) const;

		const std::vector<SkeletonSegment>& GetSkeletonSegments() const {
			return _skeletonSegments;
		}

		const std::vector<SkeletonBoneMetadata>& GetBoneMetadatas() const {
			return _boneMetadatas;
		}

	private:
		std::vector<SkeletonBoneMetadata> _boneMetadatas;
		std::vector<SkeletonBone> _bones;
		std::vector<SkeletonSegment> _skeletonSegments;
	};
}
