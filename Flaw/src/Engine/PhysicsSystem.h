#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Physics.h"
#include "Components.h"

namespace flaw {
	class Scene;

	enum PhysicsEntityEventType {
		BodyTypeChanged = 1 << 0,
		NeedUpdateMassAndInertia = 1 << 1,
	};

	class PhysicsEntity {
	public:

	private:
		friend class PhysicsSystem;

		Ref<PhysicsActor> _actor;
		std::array<Ref<PhysicsShape>, (uint32_t)PhysicsShapeType::Count> _shapes;

		uint32_t _events = 0;
	};
	
	class PhysicsSystem {
	public:
		PhysicsSystem(Scene& scene);

		void Start();
		void Update();
		void End();

		PhysicsScene& GetPhysicsScene() const { return *_physicsScene; }

	private:
		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		template<typename T>
		void RegisterColliderComponent(entt::registry& registry, entt::entity entity) {
			auto it = _physicsEntities.find(entity);
			if (it == _physicsEntities.end()) {
				return;
			}

			auto& pEntt = it->second;
			auto& transComp = registry.get<TransformComponent>(entity);
			auto& colliderComp = registry.get<T>(entity);

			Ref<PhysicsShape> shape;

			if constexpr (std::is_same_v<T, BoxColliderComponent>) {
				PhysicsBoxShape::Descriptor desc;
				desc.staticFriction = colliderComp.staticFriction;
				desc.dynamicFriction = colliderComp.dynamicFriction;
				desc.restitution = colliderComp.restitution;
				desc.offset = colliderComp.offset;
				desc.size = colliderComp.size * transComp.scale;

				shape = Physics::CreateBoxShape(desc);
			}
			else if constexpr (std::is_same_v<T, SphereColliderComponent>) {
				PhysicsSphereShape::Descriptor desc;
				desc.staticFriction = colliderComp.staticFriction;
				desc.dynamicFriction = colliderComp.dynamicFriction;
				desc.restitution = colliderComp.restitution;
				desc.offset = colliderComp.offset;
				desc.radius = colliderComp.radius * glm::max(transComp.scale.x, transComp.scale.y, transComp.scale.z);

				shape = Physics::CreateSphereShape(desc);
			}
			else if constexpr (std::is_same_v<T, MeshColliderComponent>) {
				PhysicsMeshShape::Descriptor desc;
				desc.staticFriction = colliderComp.staticFriction;
				desc.dynamicFriction = colliderComp.dynamicFriction;
				desc.restitution = colliderComp.restitution;
				// TODO: Implement mesh collider registration
			}
			else {
				static_assert(false, "Unsupported collider component type");
			}

			if (!shape) {
				return;
			}

			pEntt._actor->AttatchShape(shape);
			pEntt._shapes[(uint32_t)shape->GetShapeType()] = shape;
			pEntt._events |= PhysicsEntityEventType::NeedUpdateMassAndInertia;

			if (!pEntt._actor->IsJoined()) {
				_physicsScene->JoinActor(pEntt._actor);
			}
		}

		template<typename T>
		void UnregisterColliderComponent(entt::registry& registry, entt::entity entity) {
			auto it = _physicsEntities.find(entity);
			if (it == _physicsEntities.end()) {
				return;
			}

			auto& pEntt = it->second;
			auto& colliderComp = registry.get<T>(entity);

			Ref<PhysicsShape> shape;

			if constexpr (std::is_same_v<T, BoxColliderComponent>) {
				shape = pEntt._shapes[(uint32_t)PhysicsShapeType::Box];
			}
			else if constexpr (std::is_same_v<T, SphereColliderComponent>) {
				shape = pEntt._shapes[(uint32_t)PhysicsShapeType::Sphere];
			}
			else if constexpr (std::is_same_v<T, MeshColliderComponent>) {
				shape = pEntt._shapes[(uint32_t)PhysicsShapeType::Mesh];
			}
			else {
				static_assert(false, "Unsupported collider component type");
			}

			if (!shape) {
				return;
			}

			pEntt._actor->DetachShape(shape);
			pEntt._shapes[(uint32_t)shape->GetShapeType()] = nullptr;
			pEntt._events |= PhysicsEntityEventType::NeedUpdateMassAndInertia;

			if (pEntt._actor->HasShapes()) {
				_physicsScene->LeaveActor(pEntt._actor);
			}
		}

		Ref<PhysicsActor> CreatePhysicsActor(const TransformComponent& transComp, const RigidbodyComponent& rigidBodyComp);

	private:
		Scene& _scene;

		Ref<PhysicsScene> _physicsScene;
		std::unordered_map<entt::entity, PhysicsEntity> _physicsEntities;
		std::vector<entt::entity> _entitiesToDestroy;
	};
}