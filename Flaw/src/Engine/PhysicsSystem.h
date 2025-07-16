#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Physics.h"
#include "Components.h"
#include "Utils/HandlerRegistry.h"

namespace flaw {
	using ColliderKey = std::pair<entt::entity, PhysicsShapeType>;

	enum PhysicsEntitySignal {
		NeedUpdateMassAndInertia = 1 << 0,
	};

	struct PhysicsEntity {
		Ref<PhysicsActor> actor;
		std::array<Ref<PhysicsShape>, (uint32_t)PhysicsShapeType::Count> shapes;

		uint32_t signals = 0;
	};
}

namespace std {
	template<>
	struct hash<flaw::ColliderKey> {
		std::size_t operator()(const flaw::ColliderKey& key) const {
			return std::hash<entt::entity>()(key.first) ^ std::hash<uint32_t>()((uint32_t)key.second);
		}
	};

	template<>
	struct hash<pair<flaw::ColliderKey, flaw::ColliderKey>> {
		std::size_t operator()(const std::pair<flaw::ColliderKey, flaw::ColliderKey>& pair) const {
			return std::hash<flaw::ColliderKey>()(pair.first) ^ std::hash<flaw::ColliderKey>()(pair.second);
		}
	};
}

namespace flaw {
	class Scene;

	class PhysicsSystem {
	public:
		PhysicsSystem(Scene& scene);

		void Start();
		void Update();
		void End();

		void RegisterOnCollisionEnterHandler(HandlerId id, const std::function<void(const CollisionInfo&)>& handler);
		void RegisterOnCollisionStayHandler(HandlerId id, const std::function<void(const CollisionInfo&)>& handler);
		void RegisterOnCollisionExitHandler(HandlerId id, const std::function<void(const CollisionInfo&)>& handler);
		void RegisterOnTriggerEnterHandler(HandlerId id, const std::function<void(const TriggerInfo&)>& handler);
		void RegisterOnTriggerStayHandler(HandlerId id, const std::function<void(const TriggerInfo&)>& handler);
		void RegisterOnTriggerExitHandler(HandlerId id, const std::function<void(const TriggerInfo&)>& handler);

		void UnregisterOnCollisionEnterHandler(HandlerId id);
		void UnregisterOnCollisionStayHandler(HandlerId id);
		void UnregisterOnCollisionExitHandler(HandlerId id);
		void UnregisterOnTriggerEnterHandler(HandlerId id);
		void UnregisterOnTriggerStayHandler(HandlerId id);
		void UnregisterOnTriggerExitHandler(HandlerId id);

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

			pEntt.actor->AttachShape(shape);
			pEntt.shapes[(uint32_t)shape->GetShapeType()] = shape;
			pEntt.signals |= PhysicsEntitySignal::NeedUpdateMassAndInertia;

			if (!pEntt.actor->IsJoined()) {
				_physicsScene->JoinActor(pEntt.actor);
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
				shape = pEntt.shapes[(uint32_t)PhysicsShapeType::Box];
			}
			else if constexpr (std::is_same_v<T, SphereColliderComponent>) {
				shape = pEntt.shapes[(uint32_t)PhysicsShapeType::Sphere];
			}
			else if constexpr (std::is_same_v<T, MeshColliderComponent>) {
				shape = pEntt.shapes[(uint32_t)PhysicsShapeType::Mesh];
			}
			else {
				static_assert(false, "Unsupported collider component type");
			}

			if (!shape) {
				return;
			}

			pEntt.actor->DetachShape(shape);
			pEntt.shapes[(uint32_t)shape->GetShapeType()] = nullptr;
			pEntt.signals |= PhysicsEntitySignal::NeedUpdateMassAndInertia;

			if (!pEntt.actor->HasShapes()) {
				_physicsScene->LeaveActor(pEntt.actor);
			}
		}

		bool CheckPhysicsContactValidity(const PhysicsContact& contact) const;
		void FillCollisionInfo(PhysicsContact& contact, CollisionInfo& collisionInfo) const;

		void HandleContactEnter(PhysicsContact& contact);
		void HandleContactUpdate(PhysicsContact& contact);
		void HandleContactExit(PhysicsContact& contact);

		void DispatchCollisionEnter();
		void DispatchCollisionExit();
		void DispatchCollisionStay();

		void UpdateActiveCollisionEntities();

		void FillTriggerInfo(PhysicsTrigger& trigger, TriggerInfo& triggerInfo) const;

		void HandleTriggerEnter(PhysicsTrigger& trigger);
		void HandleTriggerStay();
		void HandleTriggerExit(PhysicsTrigger& trigger);

		Ref<PhysicsActor> CreatePhysicsActor(entt::entity entity, const TransformComponent& transComp, const RigidbodyComponent& rigidBodyComp);

		void UpdatePhysicsEntity(PhysicsEntity& pEntt, TransformComponent& transComp, RigidbodyComponent& rigidBodyComp);
		void UpdatePhysicsEntityAsStatic(PhysicsEntity& pEntt, TransformComponent& transComp, RigidbodyComponent& rigidBodyComp);
		void UpdatePhysicsEntityAsDynamic(PhysicsEntity& pEntt, TransformComponent& transComp, RigidbodyComponent& rigidBodyComp);

	private:
		Scene& _scene;

		Ref<PhysicsScene> _physicsScene;
		std::unordered_map<entt::entity, PhysicsEntity> _physicsEntities;
		std::unordered_set<PhysicsActor*> _validActors;
		std::vector<PhysicsEntity> _physicsEntitiesToDestroy;

		std::unordered_map<ColliderKey, std::unordered_map<ColliderKey, CollisionInfo>> _enteringCollisionEntities;
		std::unordered_map<ColliderKey, std::unordered_set<ColliderKey>> _exitingCollisionEntities;
		std::unordered_map<ColliderKey, std::unordered_map<ColliderKey, CollisionInfo>> _activeCollisionEntities;

		std::unordered_map<ColliderKey, std::unordered_map<ColliderKey, TriggerInfo>> _triggeredEntities;

		HandlerRegistry<void, const CollisionInfo&> _onCollisionEnterHandlers;
		HandlerRegistry<void, const CollisionInfo&> _onCollisionStayHandlers;
		HandlerRegistry<void, const CollisionInfo&> _onCollisionExitHandlers;
		HandlerRegistry<void, const TriggerInfo&> _onTriggerEnterHandlers;
		HandlerRegistry<void, const TriggerInfo&> _onTriggerStayHandlers;
		HandlerRegistry<void, const TriggerInfo&> _onTriggerExitHandlers;
	};
}