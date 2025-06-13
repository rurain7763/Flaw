#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Physics.h"
#include "Components.h"

namespace flaw {
	class Scene;
	
	class PhysicsSystem {
	public:
		PhysicsSystem(Scene& scene);

		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		void Start();
		void Update();
		void End();

	private:
		PhysicsActor* CreatePhysicsActor(const entt::entity& entity, const TransformComponent& transComp, const RigidbodyComponent& rigidBodyComp);

	private:
		Scene& _scene;

		std::unordered_map<entt::entity, PhysicsActor*> _actors;
	};
}