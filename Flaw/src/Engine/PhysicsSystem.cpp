#include "pch.h"
#include "PhysicsSystem.h"
#include "Scene.h"
#include "Time/Time.h"

namespace flaw {
	PhysicsSystem::PhysicsSystem(Scene& scene)
		: _scene(scene)
	{
	}

	Ref<PhysicsActor> PhysicsSystem::CreatePhysicsActor(entt::entity entity, const TransformComponent& transComp, const RigidbodyComponent& rigidBodyComp) {
		if (rigidBodyComp.bodyType == PhysicsBodyType::Static) {
			PhysicsActorStatic::Descriptor desc;
			desc.position = transComp.position;
			desc.rotation = transComp.rotation;
			desc.userData = (void*)entity;

			return Physics::CreateActorStatic(desc);
		}
		else if (rigidBodyComp.bodyType == PhysicsBodyType::Dynamic) {
			PhysicsActorDynamic::Descriptor desc;
			desc.position = transComp.position;
			desc.rotation = transComp.rotation;
			desc.density = rigidBodyComp.mass;
			desc.userData = (void*)entity;

			return Physics::CreateActorDynamic(desc);
		}

		throw std::runtime_error("Unsupported body type for physics actor creation.");
	}
	
	void PhysicsSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		if (!registry.all_of<TransformComponent, RigidbodyComponent>(entity)) {
			return;
		}

		auto& transComp = registry.get<TransformComponent>(entity);
		auto& rigidBodyComp = registry.get<RigidbodyComponent>(entity);

		Ref<PhysicsActor> newActor = CreatePhysicsActor(entity, transComp, rigidBodyComp);

		auto& pEntt = _physicsEntities[entity];
		pEntt.actor = newActor;
		pEntt.shapes.fill(nullptr);

		_validActors.insert(newActor.get());

		if (registry.any_of<BoxColliderComponent>(entity)) {
			RegisterColliderComponent<BoxColliderComponent>(registry, entity);
		}

		if (registry.any_of<SphereColliderComponent>(entity)) {
			RegisterColliderComponent<SphereColliderComponent>(registry, entity);
		}

		if (registry.any_of<MeshColliderComponent>(entity)) {
			RegisterColliderComponent<MeshColliderComponent>(registry, entity);
		}
	}

	void PhysicsSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		auto it = _physicsEntities.find(entity);
		if (it == _physicsEntities.end()) {
			return;
		}

		_validActors.erase(it->second.actor.get());

		_physicsEntitiesToDestroy.emplace_back(std::move(it->second));

		_physicsEntities.erase(it);
	}

	void PhysicsSystem::Start() {
		auto& registry = _scene.GetRegistry();

		PhysicsScene::Descriptor sceneDesc;
		sceneDesc.gravity = vec3(0.0f, -9.81f, 0.0f); // Default gravity

		_physicsScene = Physics::CreateScene(sceneDesc);
		_physicsScene->SetOnContactEnter(std::bind(&PhysicsSystem::HandleContactEnter, this, std::placeholders::_1));
		_physicsScene->SetOnContactUpdate(std::bind(&PhysicsSystem::HandleContactUpdate, this, std::placeholders::_1));
		_physicsScene->SetOnContactExit(std::bind(&PhysicsSystem::HandleContactExit, this, std::placeholders::_1));
		_physicsScene->SetOnTriggerEnter(std::bind(&PhysicsSystem::HandleTriggerEnter, this, std::placeholders::_1));
		_physicsScene->SetOnTriggerExit(std::bind(&PhysicsSystem::HandleTriggerExit, this, std::placeholders::_1));

		for (auto&& [entity, transComp, rigidBodyComp] : registry.view<TransformComponent, RigidbodyComponent>().each()) {
			RegisterEntity(registry, entity);
		}

		registry.on_construct<TransformComponent>().connect<&PhysicsSystem::RegisterEntity>(*this);
		registry.on_destroy<TransformComponent>().connect<&PhysicsSystem::UnregisterEntity>(*this);
		registry.on_construct<RigidbodyComponent>().connect<&PhysicsSystem::RegisterEntity>(*this);
		registry.on_destroy<RigidbodyComponent>().connect<&PhysicsSystem::UnregisterEntity>(*this);
		registry.on_construct<BoxColliderComponent>().connect<&PhysicsSystem::RegisterColliderComponent<BoxColliderComponent>>(*this);
		registry.on_destroy<BoxColliderComponent>().connect<&PhysicsSystem::UnregisterColliderComponent<BoxColliderComponent>>(*this);
		registry.on_construct<SphereColliderComponent>().connect<&PhysicsSystem::RegisterColliderComponent<SphereColliderComponent>>(*this);
		registry.on_destroy<SphereColliderComponent>().connect<&PhysicsSystem::UnregisterColliderComponent<SphereColliderComponent>>(*this);
		registry.on_construct<MeshColliderComponent>().connect<&PhysicsSystem::RegisterColliderComponent<MeshColliderComponent>>(*this);
		registry.on_destroy<MeshColliderComponent>().connect<&PhysicsSystem::UnregisterColliderComponent<MeshColliderComponent>>(*this);
	}

	bool PhysicsSystem::CheckPhysicsContactValidity(const PhysicsContact& contact) const {
		return _validActors.find(contact.actor) != _validActors.end() && _validActors.find(contact.otherActor) != _validActors.end();
	}

	void PhysicsSystem::FillCollisionInfo(PhysicsContact& contact, CollisionInfo& collisionInfo) const {
		entt::entity entity0 = (entt::entity)(uint32_t)contact.actor->GetUserData();
		entt::entity entity1 = (entt::entity)(uint32_t)contact.otherActor->GetUserData();

		collisionInfo.entity0 = Entity(entity0, &_scene);
		collisionInfo.shapeType0 = contact.shape->GetShapeType();
		collisionInfo.entity1 = Entity(entity1, &_scene);
		collisionInfo.shapeType1 = contact.otherShape->GetShapeType();
		collisionInfo.contactPoints = std::move(contact.contactPoints);
	}

	void PhysicsSystem::HandleContactEnter(PhysicsContact& contact) {
		if (!CheckPhysicsContactValidity(contact)) {
			return;
		}

		CollisionInfo collisionInfo;
		FillCollisionInfo(contact, collisionInfo);

		ColliderKey key0 = { collisionInfo.entity0, collisionInfo.shapeType0 };
		ColliderKey key1 = { collisionInfo.entity1, collisionInfo.shapeType1 };

		auto enteringPair = std::minmax(key0, key1);
		_enteringCollisionEntities[enteringPair.first][enteringPair.second] = collisionInfo;
	}

	void PhysicsSystem::HandleContactUpdate(PhysicsContact& contact) {
		if (!CheckPhysicsContactValidity(contact)) {
			return;
		}

		CollisionInfo collisionInfo;
		FillCollisionInfo(contact, collisionInfo);

		ColliderKey key0 = { collisionInfo.entity0, collisionInfo.shapeType0 };
		ColliderKey key1 = { collisionInfo.entity1, collisionInfo.shapeType1 };

		auto collidedPair = std::minmax(key0, key1);
		_activeCollisionEntities[collidedPair.first][collidedPair.second] = collisionInfo;
	}

	void PhysicsSystem::HandleContactExit(PhysicsContact& contact) {
		if (!CheckPhysicsContactValidity(contact)) {
			return;
		}

		CollisionInfo collisionInfo;
		FillCollisionInfo(contact, collisionInfo);

		ColliderKey key0 = { collisionInfo.entity0, collisionInfo.shapeType0 };
		ColliderKey key1 = { collisionInfo.entity1, collisionInfo.shapeType1 };

		auto exitingPair = std::minmax(key0, key1);
		_exitingCollisionEntities[exitingPair.first].insert(exitingPair.second);
	}

	void PhysicsSystem::FillTriggerInfo(PhysicsTrigger& trigger, TriggerInfo& triggerInfo) const {
		entt::entity entity0 = (entt::entity)(uint32_t)trigger.actor->GetUserData();
		entt::entity entity1 = (entt::entity)(uint32_t)trigger.otherActor->GetUserData();

		triggerInfo.entity0 = Entity(entity0, &_scene);
		triggerInfo.shapeType0 = trigger.shape->GetShapeType();
		triggerInfo.entity1 = Entity(entity1, &_scene);
		triggerInfo.shapeType1 = trigger.otherShape->GetShapeType();
	}

	void PhysicsSystem::HandleTriggerEnter(PhysicsTrigger& trigger) {
		TriggerInfo triggerInfo;
		FillTriggerInfo(trigger, triggerInfo);

		ColliderKey key0 = { triggerInfo.entity0, triggerInfo.shapeType0 };
		ColliderKey key1 = { triggerInfo.entity1, triggerInfo.shapeType1 };

		_triggeredEntities[key0][key1] = triggerInfo;
		_triggeredEntities[key1][key0] = triggerInfo;

		_onTriggerEnterHandlers.Invoke(triggerInfo);
	}

	void PhysicsSystem::HandleTriggerStay() {
		std::unordered_set<std::pair<ColliderKey, ColliderKey>> processedPairs;
		for (const auto& [key, triggeredEntities] : _triggeredEntities) {
			for (const auto& [entity1, triggerInfo] : triggeredEntities) {
				auto orderedPair = std::minmax(key, entity1);
				if (processedPairs.find(orderedPair) != processedPairs.end()) {
					continue; // Skip already processed pairs
				}

				_onTriggerStayHandlers.Invoke(triggerInfo);
			}
		}
	}

	void PhysicsSystem::HandleTriggerExit(PhysicsTrigger& trigger) {
		TriggerInfo triggerInfo;
		FillTriggerInfo(trigger, triggerInfo);

		ColliderKey key0 = { triggerInfo.entity0, triggerInfo.shapeType0 };
		ColliderKey key1 = { triggerInfo.entity1, triggerInfo.shapeType1 };

		_triggeredEntities[key0].erase(key1);
		_triggeredEntities[key1].erase(key0);

		_onTriggerExitHandlers.Invoke(triggerInfo);
	}

	void PhysicsSystem::Update() {
		auto& registry = _scene.GetRegistry();

		for (auto&& [entity, transComp, boxColliderComp] : registry.view<TransformComponent, BoxColliderComponent>().each()) {
			auto it = _physicsEntities.find(entity);
			if (it == _physicsEntities.end()) {
				continue;
			}

			auto& pEntt = it->second;
			auto boxShape = std::dynamic_pointer_cast<PhysicsBoxShape>(pEntt.shapes[(uint32_t)PhysicsShapeType::Box]);
			
			bool needUpdate = false;
			needUpdate |= boxShape->SetOffset(boxColliderComp.offset);
			needUpdate |= boxShape->SetSize(boxColliderComp.size * transComp.scale);

			if (needUpdate) {
				pEntt.signals |= PhysicsEntitySignal::NeedUpdateMassAndInertia;
			}
		}

		for (auto&& [entity, transComp, sphereColliderComp] : registry.view<TransformComponent, SphereColliderComponent>().each()) {
			auto it = _physicsEntities.find(entity);
			if (it == _physicsEntities.end()) {
				continue;
			}

			auto& pEntt = it->second;
			auto sphereShape = std::dynamic_pointer_cast<PhysicsSphereShape>(pEntt.shapes[(uint32_t)PhysicsShapeType::Sphere]);

			bool needUpdate = false;
			needUpdate |= sphereShape->SetOffset(sphereColliderComp.offset);
			needUpdate |= sphereShape->SetRadius(sphereColliderComp.radius * glm::max(transComp.scale.x, transComp.scale.y, transComp.scale.z));
		
			if (needUpdate) {
				pEntt.signals |= PhysicsEntitySignal::NeedUpdateMassAndInertia;
			}
		}

		for (auto&& [entity, transformComp, rigidBodyComp] : registry.view<TransformComponent, RigidbodyComponent>().each()) {
			auto it = _physicsEntities.find(entity);

			if (it == _physicsEntities.end()) {
				continue;
			}

			auto& pEntt = it->second;

			UpdatePhysicsEntity(pEntt, transformComp, rigidBodyComp);

			if (rigidBodyComp.bodyType == PhysicsBodyType::Static) {
				UpdatePhysicsEntityAsStatic(pEntt, transformComp, rigidBodyComp);
			}
			else if (rigidBodyComp.bodyType == PhysicsBodyType::Dynamic) {
				UpdatePhysicsEntityAsDynamic(pEntt, transformComp, rigidBodyComp);
			}

			pEntt.signals = 0;
		}

		_physicsScene->Update(Time::DeltaTime());

		DispatchCollisionEnter();
		DispatchCollisionExit();
		DispatchCollisionStay();

		HandleTriggerStay();

		_physicsEntitiesToDestroy.clear();

		UpdateActiveCollisionEntities();
	}

	void PhysicsSystem::UpdatePhysicsEntity(PhysicsEntity& pEntt, TransformComponent& transComp, RigidbodyComponent& rigidBodyComp) {
		if (rigidBodyComp.bodyType != pEntt.actor->GetBodyType()) {
			if (pEntt.actor->IsJoined()) {
				_physicsScene->LeaveActor(pEntt.actor);
			}

			entt::entity entity = (entt::entity)(uint32_t)pEntt.actor->GetUserData();

			auto newActor = CreatePhysicsActor(entity, transComp, rigidBodyComp);
			for (auto& shape : pEntt.shapes) {
				if (shape) {
					newActor->AttachShape(shape);
				}
			}

			_validActors.erase(pEntt.actor.get());
			pEntt.actor = newActor;
			_validActors.insert(newActor.get());

			if (pEntt.actor->HasShapes()) {
				_physicsScene->JoinActor(pEntt.actor);
			}
		}
	}

	void PhysicsSystem::UpdatePhysicsEntityAsStatic(PhysicsEntity& pEntt, TransformComponent& transComp, RigidbodyComponent& rigidBodyComp) {
		auto staticActor = std::dynamic_pointer_cast<PhysicsActorStatic>(pEntt.actor);
		Entity entity((entt::entity)(uint32_t)staticActor->GetUserData(), &_scene);

		_scene.UpdateTransformImmediate(entity);

		vec3 position, rotation;
		staticActor->GetTransform(position, rotation);

		mat4 parnetWorldMatrix = entity.HasParent() ? entity.GetParent().GetComponent<TransformComponent>().worldTransform : mat4(1.0);
		mat4 localMatrix = glm::inverse(parnetWorldMatrix) * ModelMatrix(position, rotation, transComp.scale);
		ExtractModelMatrix(localMatrix, transComp.position, transComp.rotation, transComp.scale);
		transComp.dirty = true;
	}

	void PhysicsSystem::UpdatePhysicsEntityAsDynamic(PhysicsEntity& pEntt, TransformComponent& transComp, RigidbodyComponent& rigidBodyComp) {
		auto dynamicActor = std::dynamic_pointer_cast<PhysicsActorDynamic>(pEntt.actor);
		Entity entity((entt::entity)(uint32_t)dynamicActor->GetUserData(), &_scene);

		if (rigidBodyComp.isKinematic != dynamicActor->IsKinematic()) {
			dynamicActor->SetKinematicState(rigidBodyComp.isKinematic);
		}

		if (dynamicActor && dynamicActor->SetMass(rigidBodyComp.mass)) {
			pEntt.signals |= PhysicsEntitySignal::NeedUpdateMassAndInertia;
		}

		if (pEntt.signals & PhysicsEntitySignal::NeedUpdateMassAndInertia) {
			dynamicActor->UpdateMassAndInertia();
		}

		_scene.UpdateTransformImmediate(entity);

		if (dynamicActor->IsKinematic()) {
			dynamicActor->SetKinematicTarget(transComp.GetWorldPosition(), transComp.GetWorldRotation());
		}
		else {
			vec3 position, rotation;
			dynamicActor->GetTransform(position, rotation);

			mat4 parnetWorldMatrix = entity.HasParent() ? entity.GetParent().GetComponent<TransformComponent>().worldTransform : mat4(1.0);
			mat4 localMatrix = glm::inverse(parnetWorldMatrix) * ModelMatrix(position, rotation, transComp.scale);
			ExtractModelMatrix(localMatrix, transComp.position, transComp.rotation, transComp.scale);
			transComp.dirty = true;
		}
	}

	void PhysicsSystem::DispatchCollisionEnter() {
		for (const auto& [key0, collidedEntities] : _enteringCollisionEntities) {
			for (const auto& [key1, collisionInfo] : collidedEntities) {
				_onCollisionEnterHandlers.Invoke(collisionInfo);
			}
		}
	}

	void PhysicsSystem::DispatchCollisionExit() {
		for (const auto& [key0, exitingEntities] : _exitingCollisionEntities) {
			for (const auto& key1 : exitingEntities) {
				CollisionInfo& collisionInfo = _activeCollisionEntities[key0][key1];
				_onCollisionExitHandlers.Invoke(collisionInfo);
			}
		}
	}

	// NOTE: dispatching collision stay events for active collision entities, if not valid, remove them from the active list
	void PhysicsSystem::DispatchCollisionStay() {
		for (auto it = _activeCollisionEntities.begin(); it != _activeCollisionEntities.end(); ) {
			const auto& key0 = it->first;

			if (_physicsEntities.find(key0.first) == _physicsEntities.end()) {
				it = _activeCollisionEntities.erase(it);
				continue;
			}

			auto& innerMap = it->second;
			for (auto it2 = innerMap.begin(); it2 != innerMap.end(); ) {
				const auto& key1 = it2->first;
				const auto& collisionInfo = it2->second;

				if (_physicsEntities.find(key1.first) == _physicsEntities.end()) {
					it2 = innerMap.erase(it2);
					continue;
				}

				_onCollisionStayHandlers.Invoke(collisionInfo);

				++it2;
			}

			if (innerMap.empty()) {
				it = _activeCollisionEntities.erase(it);
				continue;
			}

			++it;
		}
	}

	void PhysicsSystem::UpdateActiveCollisionEntities() {
		for (auto& [key0, enteringEntities] : _enteringCollisionEntities) {
			for (auto& [key1, collisionInfo] : enteringEntities) {
				_activeCollisionEntities[key0][key1] = std::move(collisionInfo);
			}
		}
		_enteringCollisionEntities.clear();
		
		for (auto& [key0, exitingEntities] : _exitingCollisionEntities) {
			for (auto& key1 : exitingEntities) {
				_activeCollisionEntities[key0].erase(key1);
			}
		}
		_exitingCollisionEntities.clear();
	}

	void PhysicsSystem::End() {
		auto& registry = _scene.GetRegistry();

		registry.on_construct<TransformComponent>().disconnect<&PhysicsSystem::RegisterEntity>(*this);
		registry.on_destroy<TransformComponent>().disconnect<&PhysicsSystem::UnregisterEntity>(*this);
		registry.on_construct<RigidbodyComponent>().disconnect<&PhysicsSystem::RegisterEntity>(*this);
		registry.on_destroy<RigidbodyComponent>().disconnect<&PhysicsSystem::UnregisterEntity>(*this);
		registry.on_construct<BoxColliderComponent>().disconnect<&PhysicsSystem::RegisterColliderComponent<BoxColliderComponent>>(*this);
		registry.on_destroy<BoxColliderComponent>().disconnect<&PhysicsSystem::UnregisterColliderComponent<BoxColliderComponent>>(*this);
		registry.on_construct<SphereColliderComponent>().disconnect<&PhysicsSystem::RegisterColliderComponent<SphereColliderComponent>>(*this);
		registry.on_destroy<SphereColliderComponent>().disconnect<&PhysicsSystem::UnregisterColliderComponent<SphereColliderComponent>>(*this);
		registry.on_construct<MeshColliderComponent>().disconnect<&PhysicsSystem::RegisterColliderComponent<MeshColliderComponent>>(*this);
		registry.on_destroy<MeshColliderComponent>().disconnect<&PhysicsSystem::UnregisterColliderComponent<MeshColliderComponent>>(*this);

		for (auto& [entity, pEntt] : _physicsEntities) {
			if (pEntt.actor->IsJoined()) {
				_physicsScene->LeaveActor(pEntt.actor);
			}
		}
		_physicsEntities.clear();

		_physicsScene.reset();
	}

	void PhysicsSystem::RegisterOnCollisionEnterHandler(HandlerId id, const std::function<void(const CollisionInfo&)>& handler) {
		_onCollisionEnterHandlers.Register(id, handler);
	}

	void PhysicsSystem::RegisterOnCollisionStayHandler(HandlerId id, const std::function<void(const CollisionInfo&)>& handler) {
		_onCollisionStayHandlers.Register(id, handler);
	}

	void PhysicsSystem::RegisterOnCollisionExitHandler(HandlerId id, const std::function<void(const CollisionInfo&)>& handler) {
		_onCollisionExitHandlers.Register(id, handler);
	}

	void PhysicsSystem::RegisterOnTriggerEnterHandler(HandlerId id, const std::function<void(const TriggerInfo&)>& handler) {
		_onTriggerEnterHandlers.Register(id, handler);
	}

	void PhysicsSystem::RegisterOnTriggerStayHandler(HandlerId id, const std::function<void(const TriggerInfo&)>& handler) {
		_onTriggerStayHandlers.Register(id, handler);
	}

	void PhysicsSystem::RegisterOnTriggerExitHandler(HandlerId id, const std::function<void(const TriggerInfo&)>& handler) {
		_onTriggerExitHandlers.Register(id, handler);
	}

	void PhysicsSystem::UnregisterOnCollisionEnterHandler(HandlerId id) {
		_onCollisionEnterHandlers.Unregister(id);
	}

	void PhysicsSystem::UnregisterOnCollisionStayHandler(HandlerId id) {
		_onCollisionStayHandlers.Unregister(id);
	}

	void PhysicsSystem::UnregisterOnCollisionExitHandler(HandlerId id) {
		_onCollisionExitHandlers.Unregister(id);
	}

	void PhysicsSystem::UnregisterOnTriggerEnterHandler(HandlerId id) {
		_onTriggerEnterHandlers.Unregister(id);
	}

	void PhysicsSystem::UnregisterOnTriggerStayHandler(HandlerId id) {
		_onTriggerStayHandlers.Unregister(id);
	}

	void PhysicsSystem::UnregisterOnTriggerExitHandler(HandlerId id) {
		_onTriggerExitHandlers.Unregister(id);
	}
}