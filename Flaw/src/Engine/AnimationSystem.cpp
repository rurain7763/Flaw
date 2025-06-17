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
		auto& registry = _scene.GetRegistry();
		registry.on_construct<SkeletalMeshComponent>().connect<&AnimationSystem::RegisterEntity>(*this);
		registry.on_destroy<SkeletalMeshComponent>().connect<&AnimationSystem::UnregisterEntity>(*this);
	}

	AnimationSystem::~AnimationSystem() {
		auto& registry = _scene.GetRegistry();
		registry.on_construct<SkeletalMeshComponent>().disconnect<&AnimationSystem::RegisterEntity>(*this);
		registry.on_destroy<SkeletalMeshComponent>().disconnect<&AnimationSystem::UnregisterEntity>(*this);
	}

	void AnimationSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);

		if (registry.any_of<SkeletalMeshComponent>(entity)) {
			auto& skeletalMeshComp = registry.get<SkeletalMeshComponent>(entity);
			_skeletonAnimations[entity] = CreateRef<SkeletalAnimationData>();
		}
	}

	void AnimationSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		_skeletonAnimations.erase(entity);
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
			auto skeletalAnimData = _skeletonAnimations[entity];
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
			const auto& animationHandles = skeletonAsset->GetAnimationHandles();
			if (animationHandles.empty()) {
				continue;
			}

			// TODO: Animation test
#if false
			if (animationHandles.size() >= 1) {
				auto animationAsset = AssetManager::GetAsset<SkeletalAnimationAsset>(animationHandles[0]);
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
			}
#else
			if (animationHandles.size() >= 2) {
				auto animationAsset1 = AssetManager::GetAsset<SkeletalAnimationAsset>(animationHandles[0]);
				auto animationAsset2 = AssetManager::GetAsset<SkeletalAnimationAsset>(animationHandles[1]);

				if (animationAsset1 == nullptr || animationAsset2 == nullptr) {
					continue;
				}

				Ref<SkeletalAnimation> animation1 = animationAsset1->GetAnimation();
				Ref<SkeletalAnimation> animation2 = animationAsset2->GetAnimation();

				skeletalAnimData->_animationTime += Time::DeltaTime();
				if (skeletalAnimData->_animationTime > animation1->GetDurationSec()) {
					skeletalAnimData->_animationTime = 0.0f;
				}

				float normalizedTime = skeletalAnimData->_animationTime / animation1->GetDurationSec();

				_app.AddAsyncTask([animation1, animation2, skeleton, skeletalAnimData, normalizedTime, blendFactor = skeletalMeshComp.blendFactor]() {
					std::lock_guard<std::mutex> lock(skeletalAnimData->_mutex);
					skeleton->GetBlendedAnimationMatrices(animation1, animation2, normalizedTime, blendFactor, skeletalAnimData->_animationMatrices);
					skeletalAnimData->_animationMatricesDirty = true;
				});

				skeletalAnimData->_boneMatricesSB = skeletalAnimData->_animationMatricesSB;
			}
#endif
		}
	}
}
