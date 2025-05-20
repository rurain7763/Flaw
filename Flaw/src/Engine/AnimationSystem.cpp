#include "pch.h"
#include "AnimationSystem.h"
#include "Components.h"
#include "Skeleton.h"
#include "AssetManager.h"
#include "Assets.h"

namespace flaw {
	AnimationSystem::AnimationSystem(Scene& scene)
		: _scene(scene)
	{
	}

	void AnimationSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);

		if (registry.any_of<SkeletalMeshComponent>(entity)) {
			auto& skeletalMeshComp = registry.get<SkeletalMeshComponent>(entity);
			_skeletonAnimations[enttComp.uuid] = SkeletalAnimationData();
		}
	}

	void AnimationSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);
		_skeletonAnimations.erase(enttComp.uuid);
	}

	void AnimationSystem::Update() {
		for (auto&& [entity, enttComp, transformComp, skeletalMeshComp] : _scene.GetRegistry().view<EntityComponent, TransformComponent, SkeletalMeshComponent>().each()) {
			auto meshAsset = AssetManager::GetAsset<SkeletalMeshAsset>(skeletalMeshComp.mesh);
			if (meshAsset == nullptr) {
				continue;
			}
			
			auto& skeletalAnimData = _skeletonAnimations[enttComp.uuid];
			skeletalAnimData.bindingPosMatrices.clear();
			for (const auto& skeletonHandle : meshAsset->GetSkeletonHandles()) {
				auto skeletonAsset = AssetManager::GetAsset<SkeletonAsset>(skeletonHandle);
				if (skeletonAsset == nullptr) {
					continue;
				}

				Ref<Skeleton> skeleton = skeletonAsset->GetSkeleton();

				skeletalAnimData.bindingPosMatrices[skeleton] = skeleton->GetBindingPosGPUBuffer();
			}
		}
	}
}
