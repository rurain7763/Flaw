#include "pch.h"
#include "AnimationSystem.h"
#include "Components.h"
#include "Skeleton.h"
#include "AssetManager.h"
#include "Assets.h"
#include "Time/Time.h"
#include "Application.h"
#include "Components.h"

namespace flaw {
	AnimationSystem::AnimationSystem(Application& app, Scene& scene)
		: _app(app)
		, _scene(scene)
	{
	}

	void AnimationSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);
		auto& skeletalMeshComp = registry.get<AnimatorComponent>(entity);

		// TODO: get animator asset and get animator from asset, but for now just test with test animator
		static Scope<Animator> animator;
		if (!animator) {
			Ref<SkeletonAsset> skeletonAsset = AssetManager::GetAsset<SkeletonAsset>(skeletalMeshComp.skeletonAsset);
			if (!skeletonAsset) {
				return;
			}

			Animator::Descriptor desc = {};
			desc.skeleton = skeletonAsset->GetSkeleton();
			
			for (auto& animHandle : skeletonAsset->GetAnimationHandles()) {
				auto animAsset = AssetManager::GetAsset<SkeletalAnimationAsset>(animHandle);
				if (!animAsset) {
					continue;
				}

				Ref<AnimatorAnimation1D> anim = CreateRef<AnimatorAnimation1D>(animAsset->GetAnimation());

				Ref<AnimatorState> state = CreateRef<AnimatorState>();
				state->SetAnimation(anim);
				state->SetLoop(true);

				desc.states.push_back(state);
			}

			desc.defaultStateIndex = 0;

			animator = CreateScope<Animator>(desc);
		}

		auto it = _runtimeAnimators.emplace(std::piecewise_construct, std::forward_as_tuple(entity), std::forward_as_tuple(*animator));

		auto& runtimeAnimator = it.first->second;
		runtimeAnimator.SetToDefaultState();
	}

	void AnimationSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		_runtimeAnimators.erase(entity);
	}

	void AnimationSystem::Start() {
		auto& registry = _scene.GetRegistry();

		for (auto&& [entity, animatorComp] : registry.view<AnimatorComponent>().each()) {
			RegisterEntity(registry, entity);
		}

		registry.on_construct<AnimatorComponent>().connect<&AnimationSystem::RegisterEntity>(*this);
		registry.on_destroy<AnimatorComponent>().connect<&AnimationSystem::UnregisterEntity>(*this);
	}

	void AnimationSystem::Update() {
		for (auto&& [entity, animatorComp] : _scene.GetRegistry().view<AnimatorComponent>().each()) {
			auto it = _runtimeAnimators.find(entity);
			if (it == _runtimeAnimators.end()) {
				continue; // Entity not registered
			}

			auto& runtimeAnimator = it->second;
			runtimeAnimator.Update(Time::DeltaTime());
		}
	}

	void AnimationSystem::End() {
		auto& registry = _scene.GetRegistry();

		registry.on_construct<AnimatorComponent>().disconnect<&AnimationSystem::RegisterEntity>(*this);
		registry.on_destroy<AnimatorComponent>().disconnect<&AnimationSystem::UnregisterEntity>(*this);

		_runtimeAnimators.clear();
	}

	bool AnimationSystem::HasAnimatorRuntime(entt::entity entity) const {
		return _runtimeAnimators.find(entity) != _runtimeAnimators.end();
	}

	AnimatorRuntime& AnimationSystem::GetAnimatorRuntime(entt::entity entity) {
		return _runtimeAnimators.at(entity);
	}
}
