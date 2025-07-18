#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Graphics.h"
#include "Utils/UUID.h"
#include "Skeleton.h"
#include "Animator.h"

namespace flaw {
	class Application;
	class Scene;

	struct AnimatorJobContext {
		Ref<AnimatorRuntime> runtimeAnimator;

		std::vector<mat4> animatedBoneMatrices0;
		std::vector<mat4> animatedBoneMatrices1;

		std::vector<mat4>* frontAnimatedBoneMatrices;
		std::vector<mat4>* backAnimatedBoneMatrices;

		std::vector<mat4> animatedSkinMatrices0;
		std::vector<mat4> animatedSkinMatrices1;

		std::vector<mat4>* frontAnimatedSkinMatrices;
		std::vector<mat4>* backAnimatedSkinMatrices;

		std::atomic<bool> isBackBufferReady;

		Ref<StructuredBuffer> animatedSkinMatricesSB;
	};

	class AnimationSystem {
	public:
		AnimationSystem(Application& app, Scene& scene);

		void Start();
		void Update();
		void End();

		bool HasAnimatorJobContext(entt::entity entity) const;
		AnimatorJobContext& GetAnimatorJobContext(entt::entity entity);

	private:
		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

	private:
		Application& _app;
		Scene& _scene;

		std::unordered_map<entt::entity, Ref<AnimatorJobContext>> _animatorJobContexts;
	};
}

