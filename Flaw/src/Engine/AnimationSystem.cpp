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

		auto context = CreateRef<AnimatorJobContext>();
		context->runtimeAnimator = CreateRef<AnimatorRuntime>(*animator);
		context->runtimeAnimator->SetToDefaultState();
		context->isBackBufferReady.store(false);

		context->animatedBoneMatrices0.resize(animator->GetSkeleton()->GetBoneCount());
		context->animatedBoneMatrices1.resize(animator->GetSkeleton()->GetBoneCount());

		context->frontAnimatedBoneMatrices = &context->animatedBoneMatrices0;
		context->backAnimatedBoneMatrices = &context->animatedBoneMatrices1;

		context->animatedSkinMatrices0.resize(animator->GetSkeleton()->GetBoneCount());
		context->animatedSkinMatrices1.resize(animator->GetSkeleton()->GetBoneCount());

		context->frontAnimatedSkinMatrices = &context->animatedSkinMatrices0;
		context->backAnimatedSkinMatrices = &context->animatedSkinMatrices1;

		StructuredBuffer::Descriptor desc = {};
		desc.elmSize = sizeof(mat4);
		desc.count = animator->GetSkeleton()->GetBoneCount();
		desc.bindFlags = BindFlag::ShaderResource;
		desc.accessFlags = AccessFlag::Write;

		context->animatedSkinMatricesSB = Graphics::CreateStructuredBuffer(desc);

		_animatorJobContexts[entity] = context;
	}

	void AnimationSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		_animatorJobContexts.erase(entity);
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
			auto it = _animatorJobContexts.find(entity);
			if (it == _animatorJobContexts.end()) {
				continue; // Entity not registered
			}

			auto context = it->second;

			_app.AddAsyncTask([context]() {
				context->runtimeAnimator->Update(Time::DeltaTime(), *context->backAnimatedBoneMatrices, *context->backAnimatedSkinMatrices);
				context->isBackBufferReady.store(true);
			});
		}
	}

	void AnimationSystem::End() {
		auto& registry = _scene.GetRegistry();

		registry.on_construct<AnimatorComponent>().disconnect<&AnimationSystem::RegisterEntity>(*this);
		registry.on_destroy<AnimatorComponent>().disconnect<&AnimationSystem::UnregisterEntity>(*this);

		_animatorJobContexts.clear();
	}

	bool AnimationSystem::HasAnimatorJobContext(entt::entity entity) const {
		return _animatorJobContexts.find(entity) != _animatorJobContexts.end();
	}

	AnimatorJobContext& AnimationSystem::GetAnimatorJobContext(entt::entity entity) {
		auto it = _animatorJobContexts.find(entity);
		if (it == _animatorJobContexts.end()) {
			throw std::runtime_error("Animator runtime not found for entity");
		}

		auto context = it->second;
		
		if (context->isBackBufferReady.exchange(false)) {
			std::swap(context->frontAnimatedBoneMatrices, context->backAnimatedBoneMatrices);
			std::swap(context->frontAnimatedSkinMatrices, context->backAnimatedSkinMatrices);
			context->animatedSkinMatricesSB->Update(context->frontAnimatedSkinMatrices->data(), context->frontAnimatedSkinMatrices->size() * sizeof(mat4));
		}

		return *context;
	}
}
