#include "pch.h"
#include "AnimationSystem.h"
#include "Components.h"
#include "Skeleton.h"
#include "AssetManager.h"
#include "Assets.h"
#include "Time/Time.h"
#include "Application.h"

namespace flaw {
	AnimationSystem::AnimationSystem(Application& app, Scene& scene)
		: _app(app)
		, _scene(scene)
	{
	}

	void AnimationSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);

		if (registry.any_of<SkeletalMeshComponent>(entity)) {
			auto& skeletalMeshComp = registry.get<SkeletalMeshComponent>(entity);
			_skeletonAnimations[(uint32_t)entity] = CreateRef<SkeletalAnimationData>();
		}
	}

	void AnimationSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		_skeletonAnimations.erase((uint32_t)entity);
	}

	void AnimationSystem::Update() {
		for (auto&& [entity, transformComp, skeletalMeshComp] : _scene.GetRegistry().view<TransformComponent, SkeletalMeshComponent>().each()) {
			auto meshAsset = AssetManager::GetAsset<SkeletalMeshAsset>(skeletalMeshComp.mesh);
			if (meshAsset == nullptr) {
				continue;
			}
			
			auto skeletonAsset = AssetManager::GetAsset<SkeletonAsset>(meshAsset->GetSkeletonHandle());
			if (skeletonAsset == nullptr) {
				continue;
			}

			Ref<Skeleton> skeleton = skeletonAsset->GetSkeleton();
			auto skeletalAnimData = _skeletonAnimations[(uint32_t)entity];
			auto bindingPosMatricesSB = skeleton->GetBindingPosGPUBuffer();

			if (skeletalAnimData->_bindingPosMatrices != bindingPosMatricesSB) {
				StructuredBuffer::Descriptor desc = {};
				desc.elmSize = sizeof(mat4);
				desc.count = skeleton->GetBoneCount();
				desc.bindFlags = BindFlag::ShaderResource;
				desc.accessFlags = AccessFlag::Write;
				desc.initialData = nullptr;

				skeletalAnimData->_animationMatricesSB = Graphics::CreateStructuredBuffer(desc);
			}
			skeletalAnimData->_bindingPosMatrices = bindingPosMatricesSB;

			// TODO: temporary animation
			skeletalAnimData->_boneMatricesSB = skeletalAnimData->_bindingPosMatrices;
			for (AssetHandle animationHandle : skeletonAsset->GetAnimationHandles()) {
				auto animationAsset = AssetManager::GetAsset<SkeletalAnimationAsset>(animationHandle);
				if (animationAsset == nullptr) {
					continue;
				}

				Ref<SkeletalAnimation> animation = animationAsset->GetAnimation();

				skeletalAnimData->_animationTime += Time::DeltaTime();
				if (skeletalAnimData->_animationTime > animation->GetDurationSec()) {
					skeletalAnimData->_animationTime = 0.0f;
				}

				_app.AddAsyncTask([animation, skeleton, skeletalAnimData]() {
					std::lock_guard<std::mutex> lock(skeletalAnimData->_mutex);
					skeleton->GetAnimationMatrices(animation, skeletalAnimData->_animationTime, skeletalAnimData->_animationMatrices);
					skeletalAnimData->_animationMatricesDirty = true;
				});

				skeletalAnimData->_boneMatricesSB = skeletalAnimData->_animationMatricesSB;

				break;
			}
		}
	}
}
