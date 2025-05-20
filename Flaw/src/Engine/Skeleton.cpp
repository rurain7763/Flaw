#include "pch.h"
#include "Skeleton.h"

namespace flaw {
	Skeleton::Skeleton(const std::vector<SkeletonBoneMetadata>& boneMetas, const std::vector<SkeletonBone>& bones) 
		: _boneMetadatas(boneMetas)
		, _bones(bones)
	{
		_skeletonSegments.resize(1);
		_skeletonSegments[0].boneStart = 0;
		_skeletonSegments[0].boneCount = bones.size();
	}

	Skeleton::Skeleton(const std::vector<SkeletonBoneMetadata>& boneMetas, const std::vector<SkeletonBone>& bones, const std::vector<SkeletonSegment>& segments)
		: _boneMetadatas(boneMetas)
		, _bones(bones)
		, _skeletonSegments(segments)
	{
	}

	void Skeleton::GetBindPosMatrices(std::vector<mat4>& out) const {
		out.clear();
		std::transform(_bones.begin(), _bones.end(), std::back_inserter(out), [](const SkeletonBone& bone) { return bone.transformMatrix; });
	}
}