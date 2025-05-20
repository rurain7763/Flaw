#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Graphics.h"
#include "Utils/UUID.h"

namespace flaw {
	class Scene;

	struct SkeletalAnimationData {
		std::vector<std::pair<int32_t, int32_t>> segments;
		std::vector<mat4> boneMatrices;

		int32_t GetSkeletonCount() const {
			return static_cast<int32_t>(segments.size());
		}

		int32_t GetSkeletonBoneCount() const {
			return boneMatrices.size();
		}

		int32_t GetSkeletonBoneCount(int32_t index) const {
			return segments[index].second;
		}

		const mat4* GetSkeletonBoneMatrices() const {
			return boneMatrices.data();
		}

		const mat4* GetSkeletonBoneMatrices(int32_t index) const {
			return &boneMatrices[segments[index].first];
		}
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

