#include "pch.h"
#include "Skeleton.h"

namespace flaw {
	void ComputeTransformationMatircesInHierachy(const Skeleton& skeleton, const std::function<mat4(int32_t)>& getNodeTransformMatrixFunc, std::vector<mat4>& result) {
		const mat4& globalInvMatrix = skeleton.GetGlobalInvMatrix();
		const auto& nodes = skeleton.GetNodes();
		const auto& boneMap = skeleton.GetBoneMap();
		
		std::vector<mat4> parentMatrices(nodes.size(), mat4(1.0));
		std::queue<int32_t> queue;
		queue.push(0);

		result.resize(nodes.size(), mat4(1.0f));
		while (!queue.empty()) {
			int32_t nodeIndex = queue.front();
			queue.pop();

			const SkeletonNode& node = nodes[nodeIndex];

			mat4 offsetMatrix = mat4(1.0);
			auto it = boneMap.find(node.name);
			if (it != boneMap.end()) {
				offsetMatrix = it->second.offsetMatrix;
			}

			mat4 transformMatrix = mat4(1.0f);
			if (node.parentIndex != -1) {
				transformMatrix = parentMatrices[node.parentIndex] * getNodeTransformMatrixFunc(nodeIndex);
			}
			else {
				transformMatrix = getNodeTransformMatrixFunc(nodeIndex);
			}

			result[nodeIndex] = globalInvMatrix * transformMatrix * offsetMatrix;
			parentMatrices[nodeIndex] = transformMatrix;

			for (int32_t childIndex : node.childrenIndices) {
				queue.push(childIndex);
			}
		}
	}

	Skeleton::Skeleton(const mat4& globalInvMatrix, const std::vector<SkeletonNode>& bones, const std::unordered_map<std::string, SkeletonBoneNode>& boneMap)
		: _globalInvMatrix(globalInvMatrix)
		, _nodes(bones)
		, _boneMap(boneMap)
	{
		GenerateBindingPosMatrices();
	}

	void Skeleton::GenerateBindingPosMatrices() {
		std::vector<mat4> transformationMatrices;
		ComputeTransformationMatircesInHierachy(*this, [this](int32_t nodeIndex) { return _nodes[nodeIndex].transformMatrix; }, transformationMatrices);

		StructuredBuffer::Descriptor desc = {};
		desc.elmSize = sizeof(mat4);
		desc.count = transformationMatrices.size();
		desc.bindFlags = BindFlag::ShaderResource;
		desc.initialData = transformationMatrices.data();

		_bindPosGPUBuffer = Graphics::CreateStructuredBuffer(desc);
	}
}