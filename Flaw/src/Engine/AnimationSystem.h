#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Graphics.h"
#include "Utils/UUID.h"
#include "Skeleton.h"

namespace flaw {
	class Scene;

	struct SkeletalAnimationData {
		Ref<StructuredBuffer> bindingPosMatrices;
		Ref<StructuredBuffer> animationMatrices;
		float animationTime = 0.0f;

		Ref<StructuredBuffer> boneMatrices;
	};

	class AnimationSystem {
	public:
		AnimationSystem(Scene& scene);

		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		void Update();

		SkeletalAnimationData& GetSkeletalAnimationData(UUID uuid) {
			return _skeletonAnimations[uuid];
		}

	private:
		static constexpr uint32_t MaxSkeletonBoneCount = 100;

		Scene& _scene;

		std::unordered_map<UUID, SkeletalAnimationData> _skeletonAnimations;
	};
}

