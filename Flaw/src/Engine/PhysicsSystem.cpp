#include "pch.h"
#include "PhysicsSystem.h"
#include "Scene.h"
#include "Time/Time.h"

namespace flaw {
	PhysicsSystem::PhysicsSystem(Scene& scene)
		: _scene(scene)
	{
	}

	Ref<PhysicsActor> PhysicsSystem::CreatePhysicsActor(const TransformComponent& transComp, const RigidbodyComponent& rigidBodyComp) {
		if (rigidBodyComp.bodyType == PhysicsBodyType::Static) {
			PhysicsActorStatic::Descriptor desc;
			desc.position = transComp.position;
			desc.rotation = transComp.rotation;
			desc.userData = nullptr; // TODO: handle user data if needed

			return Physics::CreateActorStatic(desc);
		}
		else if (rigidBodyComp.bodyType == PhysicsBodyType::Dynamic) {
			PhysicsActorDynamic::Descriptor desc;
			desc.position = transComp.position;
			desc.rotation = transComp.rotation;
			desc.density = rigidBodyComp.mass;
			desc.userData = nullptr; // TODO: handle user data if needed

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

		Ref<PhysicsActor> newActor = CreatePhysicsActor(transComp, rigidBodyComp);

		auto& pEntt = _physicsEntities[entity];
		pEntt._actor = newActor;
		pEntt._shapes.fill(nullptr);

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

		_entitiesToDestroy.push_back(entity);
	}

	void PhysicsSystem::Start() {
		auto& registry = _scene.GetRegistry();

		PhysicsScene::Descriptor sceneDesc;
		sceneDesc.gravity = vec3(0.0f, -9.81f, 0.0f); // Default gravity

		_physicsScene = Physics::CreateScene(sceneDesc);

		for (auto&& [entity, transComp, rigidBodyComp] : registry.view<TransformComponent, RigidbodyComponent>().each()) {
			RegisterEntity(registry, entity);
		}

		registry.on_construct<TransformComponent>().connect<&PhysicsSystem::RegisterEntity>(*this);
		registry.on_destroy<TransformComponent>().connect<&PhysicsSystem::UnregisterEntity>(*this);
		registry.on_construct<RigidbodyComponent>().connect<&PhysicsSystem::RegisterEntity>(*this);
		registry.on_destroy<RigidbodyComponent>().connect<&PhysicsSystem::UnregisterEntity>(*this);
		registry.on_construct<BoxColliderComponent>().connect<&PhysicsSystem::RegisterColliderComponent<BoxColliderComponent>>(*this);
		registry.on_destroy<BoxColliderComponent>().connect<&PhysicsSystem::RegisterColliderComponent<BoxColliderComponent>>(*this);
		registry.on_construct<SphereColliderComponent>().connect<&PhysicsSystem::RegisterColliderComponent<SphereColliderComponent>>(*this);
		registry.on_destroy<SphereColliderComponent>().connect<&PhysicsSystem::RegisterColliderComponent<SphereColliderComponent>>(*this);
		registry.on_construct<MeshColliderComponent>().connect<&PhysicsSystem::RegisterColliderComponent<MeshColliderComponent>>(*this);
		registry.on_destroy<MeshColliderComponent>().connect<&PhysicsSystem::RegisterColliderComponent<MeshColliderComponent>>(*this);
	}

	void PhysicsSystem::Update() {
		auto& registry = _scene.GetRegistry();

		for (auto&& [entity, transComp, boxColliderComp] : registry.view<TransformComponent, BoxColliderComponent>().each()) {
			auto it = _physicsEntities.find(entity);
			if (it == _physicsEntities.end()) {
				continue;
			}

			auto& pEntt = it->second;
			auto boxShape = std::dynamic_pointer_cast<PhysicsBoxShape>(pEntt._shapes[(uint32_t)PhysicsShapeType::Box]);
			
			bool needUpdate = false;
			needUpdate |= boxShape->SetOffset(boxColliderComp.offset);
			needUpdate |= boxShape->SetSize(boxColliderComp.size * transComp.scale);

			if (needUpdate) {
				pEntt._events |= PhysicsEntityEventType::NeedUpdateMassAndInertia;
			}
		}

		for (auto&& [entity, transComp, sphereColliderComp] : registry.view<TransformComponent, SphereColliderComponent>().each()) {
			auto it = _physicsEntities.find(entity);
			if (it == _physicsEntities.end()) {
				continue;
			}

			auto& pEntt = it->second;
			auto sphereShape = std::dynamic_pointer_cast<PhysicsSphereShape>(pEntt._shapes[(uint32_t)PhysicsShapeType::Sphere]);

			bool needUpdate = false;
			needUpdate |= sphereShape->SetOffset(sphereColliderComp.offset);
			needUpdate |= sphereShape->SetRadius(sphereColliderComp.radius * glm::max(transComp.scale.x, transComp.scale.y, transComp.scale.z));
		
			if (needUpdate) {
				pEntt._events |= PhysicsEntityEventType::NeedUpdateMassAndInertia;
			}
		}

		for (auto&& [entity, transformComp, rigidBodyComp] : registry.view<TransformComponent, RigidbodyComponent>().each()) {
			auto it = _physicsEntities.find(entity);

			if (it == _physicsEntities.end()) {
				continue;
			}

			auto& pEntt = it->second;

			if (rigidBodyComp.bodyType != pEntt._actor->GetBodyType()) {
				pEntt._events |= PhysicsEntityEventType::BodyTypeChanged;
			}

			if (rigidBodyComp.bodyType == PhysicsBodyType::Dynamic) {
				auto dynamicActor = std::dynamic_pointer_cast<PhysicsActorDynamic>(pEntt._actor);
				if (dynamicActor && dynamicActor->SetMass(rigidBodyComp.mass)) {
					pEntt._events |= PhysicsEntityEventType::NeedUpdateMassAndInertia;
				}
			}

			// NOTE: handle events
			if (pEntt._events & PhysicsEntityEventType::BodyTypeChanged) {
				if (pEntt._actor->IsJoined()) {
					_physicsScene->LeaveActor(pEntt._actor);
				}
				
				auto newActor = CreatePhysicsActor(transformComp, rigidBodyComp);
				for (auto& shape : pEntt._shapes) {
					if (shape) {
						newActor->AttatchShape(shape);
					}
				}

				pEntt._actor = newActor;
				
				if (pEntt._actor->HasShapes()) {
					_physicsScene->JoinActor(pEntt._actor);
				}
			}

			if (pEntt._events & PhysicsEntityEventType::NeedUpdateMassAndInertia) {
				auto dynamicActor = std::dynamic_pointer_cast<PhysicsActorDynamic>(pEntt._actor);
				if (dynamicActor) {
					dynamicActor->UpdateMassAndInertia();
				}
			}

			pEntt._events = 0;

			pEntt._actor->GetTransform(transformComp.position, transformComp.rotation);
			transformComp.dirty = true;
		}

		_physicsScene->Update(Time::DeltaTime());

		for (auto entity : _entitiesToDestroy) {
			_physicsEntities.erase(entity);
		}
		_entitiesToDestroy.clear();
	}

	void PhysicsSystem::End() {
		auto& registry = _scene.GetRegistry();

		registry.on_construct<TransformComponent>().disconnect<&PhysicsSystem::RegisterEntity>(*this);
		registry.on_destroy<TransformComponent>().disconnect<&PhysicsSystem::UnregisterEntity>(*this);
		registry.on_construct<RigidbodyComponent>().disconnect<&PhysicsSystem::RegisterEntity>(*this);
		registry.on_destroy<RigidbodyComponent>().disconnect<&PhysicsSystem::UnregisterEntity>(*this);
		registry.on_construct<BoxColliderComponent>().disconnect<&PhysicsSystem::RegisterColliderComponent<BoxColliderComponent>>(*this);
		registry.on_destroy<BoxColliderComponent>().disconnect<&PhysicsSystem::RegisterColliderComponent<BoxColliderComponent>>(*this);
		registry.on_construct<SphereColliderComponent>().disconnect<&PhysicsSystem::RegisterColliderComponent<SphereColliderComponent>>(*this);
		registry.on_destroy<SphereColliderComponent>().disconnect<&PhysicsSystem::RegisterColliderComponent<SphereColliderComponent>>(*this);
		registry.on_construct<MeshColliderComponent>().disconnect<&PhysicsSystem::RegisterColliderComponent<MeshColliderComponent>>(*this);
		registry.on_destroy<MeshColliderComponent>().disconnect<&PhysicsSystem::RegisterColliderComponent<MeshColliderComponent>>(*this);

		for (auto& [entity, pEntt] : _physicsEntities) {
			if (pEntt._actor->IsJoined()) {
				_physicsScene->LeaveActor(pEntt._actor);
			}
		}
		_physicsEntities.clear();

		_physicsScene.reset();
	}
}