#include "pch.h"
#include "AnimationSystem.h"
#include "Components.h"
#include "Skeleton.h"
#include "AssetManager.h"
#include "Assets.h"
#include "Time/Time.h"

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
			skeletalAnimData.boneMatrices = nullptr;

			auto skeletonAsset = AssetManager::GetAsset<SkeletonAsset>(meshAsset->GetSkeletonHandle());
			if (skeletonAsset == nullptr) {
				continue;
			}

			Ref<Skeleton> skeleton = skeletonAsset->GetSkeleton();

			skeletalAnimData.bindingPosMatrices = skeleton->GetBindingPosGPUBuffer();

			if (!skeletalAnimData.animationMatrices || skeletalAnimData.animationMatrices->Size() < sizeof(mat4) * skeleton->GetBoneCount()) {
				StructuredBuffer::Descriptor desc = {};
				desc.elmSize = sizeof(mat4);
				desc.count = skeleton->GetBoneCount();
				desc.bindFlags = BindFlag::ShaderResource;
				desc.accessFlags = AccessFlag::Write;
				desc.initialData = nullptr;

				skeletalAnimData.animationMatrices = Graphics::CreateStructuredBuffer(desc);
			}

			// TODO: temporary animation
			skeletalAnimData.boneMatrices = skeletalAnimData.bindingPosMatrices;
			for (AssetHandle animationHandle : skeletonAsset->GetAnimationHandles()) {
				auto animationAsset = AssetManager::GetAsset<SkeletalAnimationAsset>(animationHandle);
				if (animationAsset == nullptr) {
					continue;
				}

				Ref<SkeletalAnimation> animation = animationAsset->GetAnimation();

				skeletalAnimData.animationTime += Time::DeltaTime();
				if (skeletalAnimData.animationTime > animation->GetDurationSec()) {
					skeletalAnimData.animationTime = 0.0f;
				}

				std::vector<mat4> animationMatrices;
				skeleton->GetAnimationMatrices(animation, skeletalAnimData.animationTime, animationMatrices);
				skeletalAnimData.animationMatrices->Update(animationMatrices.data(), sizeof(mat4) * animationMatrices.size());

				skeletalAnimData.boneMatrices = skeletalAnimData.animationMatrices;
				break;
			}
		}
	}
}
