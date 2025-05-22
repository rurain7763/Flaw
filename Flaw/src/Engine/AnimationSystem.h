#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Graphics.h"
#include "Utils/UUID.h"
#include "Skeleton.h"

namespace flaw {
	class Application;
	class Scene;

	class SkeletalAnimationData {
	public:
		SkeletalAnimationData() = default;

		Ref<StructuredBuffer> GetBoneMatrices() {
			if (_boneMatricesSB != _bindingPosMatrices) {
				std::lock_guard<std::mutex> lock(_mutex);

				if (_animationMatricesDirty) {
					_animationMatricesSB->Update(_animationMatrices.data(), _animationMatrices.size() * sizeof(mat4));
					_animationMatricesDirty = false;
				}

				_boneMatricesSB = _animationMatricesSB;
			}

			return _boneMatricesSB;
		}

	private:
		friend class AnimationSystem;

		Ref<StructuredBuffer> _bindingPosMatrices;

		bool _animationMatricesDirty = true;
		std::vector<mat4> _animationMatrices;
		Ref<StructuredBuffer> _animationMatricesSB;

		float _animationTime = 0.0f;

		std::mutex _mutex;
		Ref<StructuredBuffer> _boneMatricesSB;
	};

	class AnimationSystem {
	public:
		AnimationSystem(Application& app, Scene& scene);

		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		void Update();

		SkeletalAnimationData& GetSkeletalAnimationData(uint32_t entt) {
			return *_skeletonAnimations[entt];
		}

	private:
		static constexpr uint32_t MaxSkeletonBoneCount = 100;

		Application& _app;
		Scene& _scene;

		std::unordered_map<uint32_t, Ref<SkeletalAnimationData>> _skeletonAnimations;
	};
}

