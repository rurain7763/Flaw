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

	class AnimationSystem {
	public:
		AnimationSystem(Application& app, Scene& scene);

		void Start();
		void Update();
		void End();

		bool HasAnimatorRuntime(entt::entity entity) const;
		AnimatorRuntime& GetAnimatorRuntime(entt::entity entity);

	private:
		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

	private:
		Application& _app;
		Scene& _scene;

		std::unordered_map<entt::entity, AnimatorRuntime> _runtimeAnimators;
	};
}

