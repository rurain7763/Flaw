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

		vec3 InterpolatePosition(float time) const;
		quat InterpolateRotation(float time) const;
		vec3 InterpolateScale(float time) const;
		mat4 GetTransformMatrix(float time) const;

		const std::string& GetNodeName() const { return _nodeName; }

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

	struct SkeletonBoneSocket {
		std::string name;
		int32_t boneIndex = -1;
		mat4 localTransform = mat4(1.0f);
	};

	template<>
	struct Serializer<SkeletonBoneSocket> {
		static void Serialize(SerializationArchive& archive, const SkeletonBoneSocket& value) {
			archive << value.name;
			archive << value.boneIndex;
			archive << value.localTransform;
		}

		static void Deserialize(SerializationArchive& archive, SkeletonBoneSocket& value) {
			archive >> value.name;
			archive >> value.boneIndex;
			archive >> value.localTransform;
		}
	};

	class Skeleton {
	public:
		struct Descriptor {
			mat4 globalInvMatrix;
			std::vector<SkeletonNode> nodes;
			std::vector<SkeletonBoneNode> bones;
			std::vector<SkeletonBoneSocket> sockets;
		};

		Skeleton(const Descriptor& desc);

		int32_t GetBoneCount() const { return _bones.size(); }

		const mat4& GetGlobalInvMatrix() const { return _globalInvMatrix; }
		const std::vector<SkeletonNode>& GetNodes() const { return _nodes; }

		bool HasSocket(const std::string& socketName) const;
		const SkeletonBoneSocket& GetSocket(const std::string& socketName) const;

		void GetBindingPoseBoneMatrices(std::vector<mat4>& out) const;
		void GetAnimatedBoneMatrices(const Ref<SkeletalAnimation>& animation, float normalizedTime, std::vector<mat4>& out) const;
		void GetBlendedAnimatedBoneMatrices(const Ref<SkeletalAnimation>& animation1, const Ref<SkeletalAnimation>& animation2, float normalizedTime, float blendFactor, std::vector<mat4>& out) const;

		void GetBindingPoseSkinMatrices(std::vector<mat4>& out) const;
		void GetAnimatedSkinMatrices(const Ref<SkeletalAnimation>& animation, float normalizedTime, std::vector<mat4>& out) const;
		void GetBlendedAnimatedSkinMatrices(const Ref<SkeletalAnimation>& animation1, const Ref<SkeletalAnimation>& animation2, float normalizedTime, float blendFactor, std::vector<mat4>& out) const;

		void GetBindingPoseBoneAndSkinMatrices(std::vector<mat4>& boneOut, std::vector<mat4>& skinOut) const;
		void GetAnimatedBoneAndSkinMatrices(const Ref<SkeletalAnimation>& animation, float normalizedTime, std::vector<mat4>& boneOut, std::vector<mat4>& skinOut) const;
		void GetBlendedAnimatedBoneAndSkinMatrices(const Ref<SkeletalAnimation>& animation1, const Ref<SkeletalAnimation>& animation2, float normalizedTime, float blendFactor, std::vector<mat4>& boneOut, std::vector<mat4>& skinOut) const;

	private:
		void ComputeMatricesInHierachy(const std::function<mat4(int32_t)>& getNodeTransformMatrixFunc, const std::function<void(const mat4&, const mat4&, int32_t)>& hanldeFunc) const;

	private:
		mat4 _globalInvMatrix = mat4(1.0f);
		std::vector<SkeletonNode> _nodes;
		
		std::vector<SkeletonBoneNode> _bones;
		std::unordered_map<std::string, int32_t> _boneNameToIndexMap;

		std::unordered_map<std::string, SkeletonBoneSocket> _socketMap;
	};
}
