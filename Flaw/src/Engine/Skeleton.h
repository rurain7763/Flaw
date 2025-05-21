#pragma once

#include "Core.h"
#include "Graphics.h"
#include "Utils/SerializationArchive.h"

namespace flaw {
	template<typename T>
	struct SkeletalAnimationNodeKey {
		float time;
		T value;
	};

	template<>
	struct Serializer<SkeletalAnimationNodeKey<vec3>> {
		static void Serialize(SerializationArchive& archive, const SkeletalAnimationNodeKey<vec3>& value) {
			archive << value.time;
			archive << value.value;
		}

		static void Deserialize(SerializationArchive& archive, SkeletalAnimationNodeKey<vec3>& value) {
			archive >> value.time;
			archive >> value.value;
		}
	};

	template<>
	struct Serializer<SkeletalAnimationNodeKey<vec4>> {
		static void Serialize(SerializationArchive& archive, const SkeletalAnimationNodeKey<vec4>& value) {
			archive << value.time;
			archive << value.value;
		}
		static void Deserialize(SerializationArchive& archive, SkeletalAnimationNodeKey<vec4>& value) {
			archive >> value.time;
			archive >> value.value;
		}
	};

	class SkeletalAnimationNode {
	public:
		SkeletalAnimationNode() = default;
		SkeletalAnimationNode(const std::string& nodeName, const std::vector<SkeletalAnimationNodeKey<vec3>>& positionKeys, const std::vector<SkeletalAnimationNodeKey<vec4>>& rotationKeys, const std::vector<SkeletalAnimationNodeKey<vec3>>& scaleKeys);

		mat4 GetTransformMatrix(float time) const;

		const std::string& GetNodeName() const { return _nodeName; }

	private:
		vec3 InterpolatePosition(float time) const;
		quat InterpolateRotation(float time) const;
		vec3 InterpolateScale(float time) const;

	private:
		friend struct Serializer<SkeletalAnimationNode>;

		std::string _nodeName;

		std::vector<SkeletalAnimationNodeKey<vec3>> _positionKeys;
		std::vector<SkeletalAnimationNodeKey<vec4>> _rotationKeys;
		std::vector<SkeletalAnimationNodeKey<vec3>> _scaleKeys;
	};

	template<>
	struct Serializer<SkeletalAnimationNode> {
		static void Serialize(SerializationArchive& archive, const SkeletalAnimationNode& value) {
			archive << value._nodeName;
			archive << value._positionKeys;
			archive << value._rotationKeys;
			archive << value._scaleKeys;
		}

		static void Deserialize(SerializationArchive& archive, SkeletalAnimationNode& value) {
			archive >> value._nodeName;
			archive >> value._positionKeys;
			archive >> value._rotationKeys;
			archive >> value._scaleKeys;
		}
	};

	class SkeletalAnimation {
	public:
		SkeletalAnimation(const std::string& name, float durationSec, const std::vector<SkeletalAnimationNode>& animationNodes);

		int32_t GetNodeIndex(const std::string& nodeName) const;

		const std::string& GetName() const { return _name; }
		float GetDurationSec() const { return _durationSec; }
		const std::vector<SkeletalAnimationNode>& GetAnimationNodes() const { return _nodes; }
		const SkeletalAnimationNode& GetAnimationNodeAt(int32_t index) const { return _nodes[index]; }

	private:
		std::string _name;
		float _durationSec = 0.0f;

		std::unordered_map<std::string, int32_t> _nodeMap;
		std::vector<SkeletalAnimationNode> _nodes;
	};

	struct SkeletonNode {
		std::string name;
		int32_t parentIndex = -1;
		mat4 transformMatrix = mat4(1.0f);
		std::vector<int32_t> childrenIndices;

		bool IsRoot() const { return parentIndex == -1; }
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
		int32_t boneIndex = -1;
		mat4 offsetMatrix = mat4(1.0f);
	};

	template<>
	struct Serializer<SkeletonBoneNode> {
		static void Serialize(SerializationArchive& archive, const SkeletonBoneNode& value) {
			archive << value.nodeIndex;
			archive << value.boneIndex;
			archive << value.offsetMatrix;
		}

		static void Deserialize(SerializationArchive& archive, SkeletonBoneNode& value) {
			archive >> value.nodeIndex;
			archive >> value.boneIndex;
			archive >> value.offsetMatrix;
		}
	};

	class Skeleton {
	public:
		Skeleton(const mat4& globalInvMatrix, const std::vector<SkeletonNode>& nodes, const std::unordered_map<std::string, SkeletonBoneNode>& boneMap);

		int32_t GetBoneCount() const { return _boneMap.size(); }

		const mat4& GetGlobalInvMatrix() const { return _globalInvMatrix; }
		const std::vector<SkeletonNode>& GetNodes() const { return _nodes; }
		const std::unordered_map<std::string, SkeletonBoneNode>& GetBoneMap() const { return _boneMap; }

		Ref<StructuredBuffer> GetBindingPosGPUBuffer() const { return _bindPosGPUBuffer; }
		void GetAnimationMatrices(const Ref<SkeletalAnimation>& animation, float timeSec, std::vector<mat4>& out) const;

	private:
		void ComputeTransformationMatircesInHierachy(const std::function<mat4(int32_t)>& getNodeTransformMatrixFunc, std::vector<mat4>& result) const;
		void GenerateBindingPosMatrices();

	private:
		mat4 _globalInvMatrix = mat4(1.0f);
		std::vector<SkeletonNode> _nodes;
		std::unordered_map<std::string, SkeletonBoneNode> _boneMap;

		Ref<StructuredBuffer> _bindPosGPUBuffer;
	};
}
