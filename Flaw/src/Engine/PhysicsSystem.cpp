#include "pch.h"
#include "PhysicsSystem.h"
#include "Scene.h"

namespace flaw {
	PhysicsSystem::PhysicsSystem(Scene& scene)
		: _scene(scene)
	{
	}

	PhysicsActor* PhysicsSystem::CreatePhysicsActor(const entt::entity& entity, const TransformComponent& transComp, const RigidbodyComponent& rigidBodyComp) {
		Entity entt(entity, &_scene);

		RigidBody rigidBody;
		rigidBody.type = rigidBodyComp.bodyType;
		rigidBody.staticFriction = rigidBodyComp.staticFriction;
		rigidBody.dynamicFriction = rigidBodyComp.dynamicFriction;
		rigidBody.restitution = rigidBodyComp.restitution;
		rigidBody.density = rigidBodyComp.density;

		ActorDescription desc;
		desc.body = &rigidBody;
		desc.userData = nullptr; // TODO: handle user data if needed

		if (entt.HasComponent<BoxColliderComponent>()) {
			auto& boxCollider = entt.GetComponent<BoxColliderComponent>();

			PhysicsBoxCollider boxColliderPhysics;
			boxColliderPhysics.position = transComp.position;
			boxColliderPhysics.rotation = transComp.rotation;
			boxColliderPhysics.size = boxCollider.size * transComp.scale;

			desc.collider = &boxColliderPhysics;
		}
		else if (entt.HasComponent<SphereColliderComponent>()) {
			auto& sphereCollider = entt.GetComponent<SphereColliderComponent>();

			PhysicsSphereCollider sphereColliderPhysics;
			sphereColliderPhysics.position = transComp.position;
			sphereColliderPhysics.rotation = transComp.rotation;
			sphereColliderPhysics.radius = sphereCollider.radius;

			desc.collider = &sphereColliderPhysics;
		}
		else if (entt.HasComponent<MeshColliderComponent>()) {
			// TODO:
			return nullptr;
		}
		else {
			return nullptr;
		}

		return Physics::CreateActor(desc);
	}
	
	void PhysicsSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto physicsActor = CreatePhysicsActor(entity, registry.get<TransformComponent>(entity), registry.get<RigidbodyComponent>(entity));
		if (physicsActor) {
			_actors[entity] = physicsActor;
		}
	}

	void PhysicsSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		auto it = _actors.find(entity);
		if (it == _actors.end()) {
			return;
		}
		PhysicsActor* actor = it->second;
		Physics::DestroyActor(actor);
		_actors.erase(it);
	}

	void PhysicsSystem::Start() {
		auto& registry = _scene.GetRegistry();

		for (auto&& [entity, transComp, rigidBodyComp] : registry.view<TransformComponent, RigidbodyComponent>().each()) {
			auto actor = CreatePhysicsActor(entity, transComp, rigidBodyComp);
			if (actor) {
				_actors[entity] = actor;
			}
		}

		registry.on_construct<RigidbodyComponent>().connect<&PhysicsSystem::RegisterEntity>(*this);
		registry.on_destroy<EntityComponent>().connect<&PhysicsSystem::UnregisterEntity>(*this);
	}

	void PhysicsSystem::Update() {
		// TODO: whate if collider type is changed? think about it later
		for (auto&& [entity, transformComp, rigidBodyComp] : _scene.GetRegistry().view<TransformComponent, RigidbodyComponent>().each()) {
			auto it = _actors.find(entity);

			if (it == _actors.end()) {
				PhysicsActor* actor = CreatePhysicsActor(entity, transformComp, rigidBodyComp);
				if (actor) {
					_actors[entity] = actor;
					it = _actors.find(entity);
				}
				else {
					continue;
				}
			}

			PhysicsActor* actor = it->second;
			actor->GetTransform(transformComp.position, transformComp.rotation);
			transformComp.dirty = true;
		}
	}

	void PhysicsSystem::End() {
		auto& registry = _scene.GetRegistry();
		registry.on_construct<RigidbodyComponent>().disconnect<&PhysicsSystem::RegisterEntity>(*this);
		registry.on_destroy<EntityComponent>().disconnect<&PhysicsSystem::UnregisterEntity>(*this);

		for (auto& [entity, actor] : _actors) {
			Physics::DestroyActor(actor);
		}
		_actors.clear();
	}
}