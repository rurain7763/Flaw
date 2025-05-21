#include "pch.h"
#include "Skeleton.h"
#include "Utils/Search.h"
#include "Log/Log.h"

namespace flaw {
	SkeletalAnimationNode::SkeletalAnimationNode(const std::string& nodeName, const std::vector<SkeletalAnimationNodeKey<vec3>>& positionKeys, const std::vector<SkeletalAnimationNodeKey<vec4>>& rotationKeys, const std::vector<SkeletalAnimationNodeKey<vec3>>& scaleKeys)
		: _nodeName(nodeName)
		, _positionKeys(positionKeys)
		, _rotationKeys(rotationKeys)
		, _scaleKeys(scaleKeys)
	{
	}

	mat4 SkeletalAnimationNode::GetTransformMatrix(float time) const {
		return ModelMatrix(InterpolatePosition(time), InterpolateRotation(time), InterpolateScale(time));
	}

	vec3 SkeletalAnimationNode::InterpolatePosition(float time) const {
		if (_positionKeys.empty()) {
			return vec3(0.0f);
		}

		if (_positionKeys.size() == 1) {
			return _positionKeys[0].value;
		}

		int32_t index = Lowerbound<SkeletalAnimationNodeKey<vec3>>(_positionKeys, [&time](const auto& pair) { return pair.time >= time; });
		if (index == 0) {
			return _positionKeys[0].value;
		}
		else if (index == _positionKeys.size()) {
			return _positionKeys.back().value;
		}

		const auto& key0 = _positionKeys[index - 1];
		const auto& key1 = _positionKeys[index];
		const float factor = (time - key0.time) / (key1.time - key0.time);

		return mix(key0.value, key1.value, factor);
	}

	quat SkeletalAnimationNode::InterpolateRotation(float time) const {
		if (_rotationKeys.empty()) {
			return quat(1, 0, 0, 0);
		}

		if (_rotationKeys.size() == 1) {
			return _rotationKeys[0].value;
		}

		int32_t index = Lowerbound<SkeletalAnimationNodeKey<vec4>>(_rotationKeys, [&time](const auto& pair) { return pair.time >= time; });
		if (index == 0) {
			const auto& key = _rotationKeys[0];
			return quat(key.value.w, key.value.x, key.value.y, key.value.z);
		}
		else if (index == _rotationKeys.size()) {
			const auto& key = _rotationKeys.back();
			return quat(key.value.w, key.value.x, key.value.y, key.value.z);
		}

		const auto& key0 = _rotationKeys[index - 1];
		const auto& key1 = _rotationKeys[index];
		const float factor = (time - key0.time) / (key1.time - key0.time);

		quat q1 = quat(key0.value.w, key0.value.x, key0.value.y, key0.value.z);
		quat q2 = quat(key1.value.w, key1.value.x, key1.value.y, key1.value.z);

		return normalize(slerp(q1, q2, factor));
	}

	vec3 SkeletalAnimationNode::InterpolateScale(float time) const {
		if (_scaleKeys.empty()) {
			return vec3(1.0f);
		}

		if (_scaleKeys.size() == 1) {
			return _scaleKeys[0].value;
		}

		int32_t index = Lowerbound<SkeletalAnimationNodeKey<vec3>>(_scaleKeys, [&time](const auto& pair) { return pair.time >= time; });
		if (index == 0) {
			return _scaleKeys[0].value;
		}
		else if (index == _scaleKeys.size()) {
			return _scaleKeys.back().value;
		}

		const auto& key0 = _scaleKeys[index - 1];
		const auto& key1 = _scaleKeys[index];
		const float factor = (time - key0.time) / (key1.time - key0.time);

		return mix(key0.value, key1.value, factor);
	}

	SkeletalAnimation::SkeletalAnimation(const std::string& name, float durationSec, const std::vector<SkeletalAnimationNode>& animationNodes)
		: _name(name)
		, _durationSec(durationSec)
		, _nodes(animationNodes)
	{
		for (size_t i = 0; i < _nodes.size(); ++i) {
			_nodeMap[_nodes[i].GetNodeName()] = i;
		}
	}

	int32_t SkeletalAnimation::GetNodeIndex(const std::string& nodeName) const {
		auto it = _nodeMap.find(nodeName);
		if (it != _nodeMap.end()) {
			return it->second;
		}
		return -1;
	}

	Skeleton::Skeleton(const mat4& globalInvMatrix, const std::vector<SkeletonNode>& bones, const std::unordered_map<std::string, SkeletonBoneNode>& boneMap)
		: _globalInvMatrix(globalInvMatrix)
		, _nodes(bones)
		, _boneMap(boneMap)
	{
		GenerateBindingPosMatrices();
	}

	void Skeleton::ComputeTransformationMatircesInHierachy(const std::function<mat4(int32_t)>& getNodeTransformMatrixFunc, std::vector<mat4>& result) const {
		std::vector<mat4> parentMatrices(_nodes.size());
		std::queue<int32_t> queue;
		queue.push(0);

		result.resize(_boneMap.size());
		while (!queue.empty()) {
			int32_t nodeIndex = queue.front();
			queue.pop();

			const auto& node = _nodes[nodeIndex];
			mat4 transformMatrix = node.IsRoot() ? getNodeTransformMatrixFunc(nodeIndex) : parentMatrices[node.parentIndex] * getNodeTransformMatrixFunc(nodeIndex);

			auto it = _boneMap.find(node.name);
			if (it != _boneMap.end()) {
				result[it->second.boneIndex] = _globalInvMatrix * transformMatrix * it->second.offsetMatrix;
			}

			parentMatrices[nodeIndex] = transformMatrix;

			for (int32_t childIndex : node.childrenIndices) {
				queue.push(childIndex);
			}
		}
	}

	void Skeleton::GenerateBindingPosMatrices() {
		std::vector<mat4> transformationMatrices;
		ComputeTransformationMatircesInHierachy([this](int32_t nodeIndex) { return _nodes[nodeIndex].transformMatrix; }, transformationMatrices);

		StructuredBuffer::Descriptor desc = {};
		desc.elmSize = sizeof(mat4);
		desc.count = transformationMatrices.size();
		desc.bindFlags = BindFlag::ShaderResource;
		desc.initialData = transformationMatrices.data();

		_bindPosGPUBuffer = Graphics::CreateStructuredBuffer(desc);
	}

	void Skeleton::GetAnimationMatrices(const Ref<SkeletalAnimation>& animation, float timeSec, std::vector<mat4>& out) const {
		ComputeTransformationMatircesInHierachy([this, &animation, &timeSec](int32_t nodeIndex) {
			const auto& skeletonNode = _nodes[nodeIndex];
			int32_t animationNodeIndex = animation->GetNodeIndex(skeletonNode.name);
			if (animationNodeIndex == -1) return mat4(1.0);
			else return animation->GetAnimationNodeAt(animationNodeIndex).GetTransformMatrix(timeSec);
		}, out);
	}
}