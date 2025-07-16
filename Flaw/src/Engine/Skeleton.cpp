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

	Skeleton::Skeleton(const Descriptor& desc)
		: _globalInvMatrix(desc.globalInvMatrix)
		, _nodes(desc.nodes)
	{
		_bones.resize(desc.bones.size());
		for (const auto& boneNode : desc.bones) {
			const auto& node = _nodes[boneNode.nodeIndex];
			_bones[boneNode.boneIndex] = boneNode;
			_boneNameToIndexMap[node.name] = boneNode.boneIndex;
		}

		for (const auto& socket : desc.sockets) {
			_socketMap[socket.name] = socket;
		}
	}

	bool Skeleton::HasSocket(const std::string& socketName) const {
		return _socketMap.find(socketName) != _socketMap.end();
	}

	const SkeletonBoneSocket& Skeleton::GetSocket(const std::string& socketName) const {
		auto it = _socketMap.find(socketName);
		if (it != _socketMap.end()) {
			return it->second;
		}

		throw std::runtime_error("Bone socket not found: " + socketName);
	}

	void Skeleton::ComputeMatricesInHierachy(const std::function<mat4(int32_t)>& getNodeTransformMatrixFunc, const std::function<void(const mat4&, const mat4&, int32_t)>& hanldeFunc) const {
		std::vector<mat4> parentMatrices(_nodes.size());

		for (int32_t i = 0; i < _nodes.size(); ++i) {
			const auto& node = _nodes[i];
			mat4 localMatrix = getNodeTransformMatrixFunc(i);
			mat4 transformMatrix = node.IsRoot() ? localMatrix : parentMatrices[node.parentIndex] * localMatrix;
			
			auto it = _boneNameToIndexMap.find(node.name);
			if (it != _boneNameToIndexMap.end()) {
				int32_t boneIndex = it->second;
				const auto& boneNode = _bones[boneIndex];

				hanldeFunc(transformMatrix, boneNode.offsetMatrix, boneIndex);
			}

			parentMatrices[i] = transformMatrix;
		}
	}

	void Skeleton::GetBindingPoseBoneMatrices(std::vector<mat4>& out) const {
		out.resize(_bones.size());
		ComputeMatricesInHierachy(
			[this](int32_t nodeIndex) { return _nodes[nodeIndex].transformMatrix; },
			[this, &out](const mat4& transformMatrix, const mat4& offsetMatrix, int32_t boneIndex) {
				out[boneIndex] = transformMatrix;
			}
		);
	}

	void Skeleton::GetAnimatedBoneMatrices(const Ref<SkeletalAnimation>& animation, float normalizedTime, std::vector<mat4>& out) const {
		const float timeSec = animation->GetDurationSec() * normalizedTime;
		out.resize(_bones.size());
		ComputeMatricesInHierachy(
			[this, &animation, &timeSec](int32_t nodeIndex) {
				int32_t animationNodeIndex = animation->GetNodeIndex(_nodes[nodeIndex].name);
				if (animationNodeIndex != -1) {
					return animation->GetAnimationNodeAt(animationNodeIndex).GetTransformMatrix(timeSec);
				}
				return _nodes[nodeIndex].transformMatrix;
			},
			[this, &out](const mat4& transformMatrix, const mat4& offsetMatrix, int32_t boneIndex) {
				out[boneIndex] = transformMatrix;
			}
		);
	}

	void Skeleton::GetBindingPoseSkinMatrices(std::vector<mat4>& out) const {
		out.resize(_bones.size());
		ComputeMatricesInHierachy(
			[this](int32_t nodeIndex) { return _nodes[nodeIndex].transformMatrix; },
			[this, &out](const mat4& transformMatrix, const mat4& offsetMatrix, int32_t boneIndex) {
				out[boneIndex] = _globalInvMatrix * transformMatrix * offsetMatrix;
			}
		);
	}

	void Skeleton::GetBlendedAnimatedBoneMatrices(const Ref<SkeletalAnimation>& animation1, const Ref<SkeletalAnimation>& animation2, float normalizedTime, float blendFactor, std::vector<mat4>& out) const {
		const float timeSec1 = animation1->GetDurationSec() * normalizedTime;
		const float timeSec2 = animation2->GetDurationSec() * normalizedTime;

		out.resize(_bones.size());
		ComputeMatricesInHierachy(
			[this, &animation1, &animation2, &normalizedTime, &blendFactor, &timeSec1, &timeSec2](int32_t nodeIndex) {
				int32_t animationNodeIndex1 = animation1->GetNodeIndex(_nodes[nodeIndex].name);
				int32_t animationNodeIndex2 = animation2->GetNodeIndex(_nodes[nodeIndex].name);
				if (animationNodeIndex1 != -1 && animationNodeIndex2 != -1) {
					const auto& animationNode1 = animation1->GetAnimationNodeAt(animationNodeIndex1);
					const auto& animationNode2 = animation2->GetAnimationNodeAt(animationNodeIndex2);
					const vec3 position = mix(animationNode1.InterpolatePosition(timeSec1), animationNode2.InterpolatePosition(timeSec2), blendFactor);
					const quat rotation = normalize(slerp(animationNode1.InterpolateRotation(timeSec1), animationNode2.InterpolateRotation(timeSec2), blendFactor));
					const vec3 scale = mix(animationNode1.InterpolateScale(timeSec1), animationNode2.InterpolateScale(timeSec2), blendFactor);
					return ModelMatrix(position, rotation, scale);
				}
				else if (animationNodeIndex1 != -1) {
					return animation1->GetAnimationNodeAt(animationNodeIndex1).GetTransformMatrix(timeSec1);
				}
				else if (animationNodeIndex2 != -1) {
					return animation2->GetAnimationNodeAt(animationNodeIndex2).GetTransformMatrix(timeSec2);
				}
				return _nodes[nodeIndex].transformMatrix;
			},
			[this, &out](const mat4& transformMatrix, const mat4& offsetMatrix, int32_t boneIndex) {
				out[boneIndex] = transformMatrix;
			}
		);
	}

	void Skeleton::GetAnimatedSkinMatrices(const Ref<SkeletalAnimation>& animation, float normalizedTime, std::vector<mat4>& out) const {
		const float timeSec = animation->GetDurationSec() * normalizedTime;
		
		out.resize(_bones.size());
		ComputeMatricesInHierachy(
			[this, &animation, &timeSec](int32_t nodeIndex) {
				int32_t animationNodeIndex = animation->GetNodeIndex(_nodes[nodeIndex].name);
				if (animationNodeIndex != -1) {
					return animation->GetAnimationNodeAt(animationNodeIndex).GetTransformMatrix(timeSec);
				}
				return _nodes[nodeIndex].transformMatrix;
			},
			[this, &out](const mat4& transformMatrix, const mat4& offsetMatrix, int32_t boneIndex) {
				out[boneIndex] = _globalInvMatrix * transformMatrix * offsetMatrix;
			}
		);
	}

	void Skeleton::GetBlendedAnimatedSkinMatrices(const Ref<SkeletalAnimation>& animation1, const Ref<SkeletalAnimation>& animation2, float normalizedTime, float blendFactor, std::vector<mat4>& out) const {
		const float timeSec1 = animation1->GetDurationSec() * normalizedTime;
		const float timeSec2 = animation2->GetDurationSec() * normalizedTime;

		out.resize(_bones.size());
		ComputeMatricesInHierachy(
			[this, &animation1, &animation2, &normalizedTime, &blendFactor, &timeSec1, &timeSec2](int32_t nodeIndex) {
				int32_t animationNodeIndex1 = animation1->GetNodeIndex(_nodes[nodeIndex].name);
				int32_t animationNodeIndex2 = animation2->GetNodeIndex(_nodes[nodeIndex].name);

				if (animationNodeIndex1 != -1 && animationNodeIndex2 != -1) {
					const auto& animationNode1 = animation1->GetAnimationNodeAt(animationNodeIndex1);
					const auto& animationNode2 = animation2->GetAnimationNodeAt(animationNodeIndex2);

					const vec3 position = mix(animationNode1.InterpolatePosition(timeSec1), animationNode2.InterpolatePosition(timeSec2), blendFactor);
					const quat rotation = normalize(slerp(animationNode1.InterpolateRotation(timeSec1), animationNode2.InterpolateRotation(timeSec2), blendFactor));
					const vec3 scale = mix(animationNode1.InterpolateScale(timeSec1), animationNode2.InterpolateScale(timeSec2), blendFactor);

					return ModelMatrix(position, rotation, scale);
				}
				else if (animationNodeIndex1 != -1) {
					return animation1->GetAnimationNodeAt(animationNodeIndex1).GetTransformMatrix(timeSec1);
				}
				else if (animationNodeIndex2 != -1) {
					return animation2->GetAnimationNodeAt(animationNodeIndex2).GetTransformMatrix(timeSec2);
				}

				return _nodes[nodeIndex].transformMatrix;
			}, 
			[this, &out](const mat4& transformMatrix, const mat4& offsetMatrix, int32_t boneIndex) {
				out[boneIndex] = _globalInvMatrix * transformMatrix * offsetMatrix;
			}
		);
	}

	void Skeleton::GetBindingPoseBoneAndSkinMatrices(std::vector<mat4>& boneOut, std::vector<mat4>& skinOut) const {
		boneOut.resize(_bones.size());
		skinOut.resize(_bones.size());

		ComputeMatricesInHierachy(
			[this](int32_t nodeIndex) { return _nodes[nodeIndex].transformMatrix; },
			[this, &boneOut, &skinOut](const mat4& transformMatrix, const mat4& offsetMatrix, int32_t boneIndex) {
				boneOut[boneIndex] = transformMatrix;
				skinOut[boneIndex] = _globalInvMatrix * transformMatrix * offsetMatrix;
			}
		);
	}

	void Skeleton::GetAnimatedBoneAndSkinMatrices(const Ref<SkeletalAnimation>& animation, float normalizedTime, std::vector<mat4>& boneOut, std::vector<mat4>& skinOut) const {
		const float timeSec = animation->GetDurationSec() * normalizedTime;
		boneOut.resize(_bones.size());
		skinOut.resize(_bones.size());
		ComputeMatricesInHierachy(
			[this, &animation, &timeSec](int32_t nodeIndex) {
				int32_t animationNodeIndex = animation->GetNodeIndex(_nodes[nodeIndex].name);
				if (animationNodeIndex != -1) {
					return animation->GetAnimationNodeAt(animationNodeIndex).GetTransformMatrix(timeSec);
				}
				return _nodes[nodeIndex].transformMatrix;
			},
			[this, &boneOut, &skinOut](const mat4& transformMatrix, const mat4& offsetMatrix, int32_t boneIndex) {
				boneOut[boneIndex] = transformMatrix;
				skinOut[boneIndex] = _globalInvMatrix * transformMatrix * offsetMatrix;
			}
		);
	}

	void Skeleton::GetBlendedAnimatedBoneAndSkinMatrices(const Ref<SkeletalAnimation>& animation1, const Ref<SkeletalAnimation>& animation2, float normalizedTime, float blendFactor, std::vector<mat4>& boneOut, std::vector<mat4>& skinOut) const {
		const float timeSec1 = animation1->GetDurationSec() * normalizedTime;
		const float timeSec2 = animation2->GetDurationSec() * normalizedTime;

		boneOut.resize(_bones.size());
		skinOut.resize(_bones.size());
		ComputeMatricesInHierachy(
			[this, &animation1, &animation2, &normalizedTime, &blendFactor, &timeSec1, &timeSec2](int32_t nodeIndex) {
				int32_t animationNodeIndex1 = animation1->GetNodeIndex(_nodes[nodeIndex].name);
				int32_t animationNodeIndex2 = animation2->GetNodeIndex(_nodes[nodeIndex].name);
				if (animationNodeIndex1 != -1 && animationNodeIndex2 != -1) {
					const auto& animationNode1 = animation1->GetAnimationNodeAt(animationNodeIndex1);
					const auto& animationNode2 = animation2->GetAnimationNodeAt(animationNodeIndex2);
					const vec3 position = mix(animationNode1.InterpolatePosition(timeSec1), animationNode2.InterpolatePosition(timeSec2), blendFactor);
					const quat rotation = normalize(slerp(animationNode1.InterpolateRotation(timeSec1), animationNode2.InterpolateRotation(timeSec2), blendFactor));
					const vec3 scale = mix(animationNode1.InterpolateScale(timeSec1), animationNode2.InterpolateScale(timeSec2), blendFactor);
					return ModelMatrix(position, rotation, scale);
				}
				else if (animationNodeIndex1 != -1) {
					return animation1->GetAnimationNodeAt(animationNodeIndex1).GetTransformMatrix(timeSec1);
				}
				else if (animationNodeIndex2 != -1) {
					return animation2->GetAnimationNodeAt(animationNodeIndex2).GetTransformMatrix(timeSec2);
				}
				return _nodes[nodeIndex].transformMatrix;
			},
			[this, &boneOut, &skinOut](const mat4& transformMatrix, const mat4& offsetMatrix, int32_t boneIndex) {
				boneOut[boneIndex] = transformMatrix;
				skinOut[boneIndex] = _globalInvMatrix * transformMatrix * offsetMatrix;
			}
		);
	}
}