#include "pch.h"
#include "Skeleton.h"

namespace flaw {
	Skeleton::Skeleton(const mat4& globalInvMatrix, const std::unordered_map<std::string, int32_t>& boneNameMap, const std::vector<SkeletonBone>& bones)
		: _globalInvMatrix(globalInvMatrix)
		, _boneNameMap(boneNameMap)
		, _bones(bones) 
	{
		GenerateBindingPosMatrices();
	}

	void Skeleton::GenerateBindingPosMatrices() {
		// Transoformation matrices를 트리를 순회하며 계산한다.
		std::vector<mat4> transformationMatrices(_bones.size());
		std::transform(_bones.begin(), _bones.end(), transformationMatrices.begin(), [this](const SkeletonBone& bone) {
			return _globalInvMatrix * bone.transformMatrix * bone.offsetMatrix;
		});

		StructuredBuffer::Descriptor desc = {};
		desc.elmSize = sizeof(mat4);
		desc.count = transformationMatrices.size();
		desc.bindFlags = BindFlag::ShaderResource;
		desc.initialData = transformationMatrices.data();

		_bindPosGPUBuffer = Graphics::CreateStructuredBuffer(desc);
	}
}