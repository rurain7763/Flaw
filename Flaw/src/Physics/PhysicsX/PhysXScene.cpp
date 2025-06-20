#include "pch.h"
#include "PhysXScene.h"
#include "PhysXContext.h"
#include "Helper.h"
#include "Log/Log.h"
#include "PhysXActors.h"

namespace flaw {
	constexpr uint8_t MaxContactCount = 32;

	void PhysXEventCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) {
		auto actor0 = pairHeader.actors[0];
		auto actor1 = pairHeader.actors[1];

		if (!actor0 || !actor1) {
			return;
		}

		PhysicsActor* physXActor0 = static_cast<PhysicsActor*>(actor0->userData);
		PhysicsActor* physXActor1 = static_cast<PhysicsActor*>(actor1->userData);

		if (!physXActor0 || !physXActor1) {
			return;
		}

		for (PxU32 i = 0; i < nbPairs; i++) {
			const PxContactPair& pair = pairs[i];

			PhysicsShape* shape0 = static_cast<PhysicsShape*>(pair.shapes[0]->userData);
			PhysicsShape* shape1 = static_cast<PhysicsShape*>(pair.shapes[1]->userData);
			if (!shape0 || !shape1) {
				continue;
			}

			PhysicsContact contact;
			contact.actor = physXActor0;
			contact.shape = shape0;
			contact.otherActor = physXActor1;
			contact.otherShape = shape1;

			if (pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
				PxContactPairPoint contactPoints[MaxContactCount];
				uint32_t contactCount =pair.extractContacts(contactPoints, MaxContactCount);

				contact.contactPoints.reserve(contactCount);
				std::transform(contactPoints, contactPoints + contactCount, std::back_inserter(contact.contactPoints), PxContactPointToContactPoint);
				
				_scene._onContactEnter(contact);
			}
			else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS) {
				PxContactPairPoint contactPoints[MaxContactCount];
				uint32_t contactCount = pair.extractContacts(contactPoints, MaxContactCount);

				contact.contactPoints.reserve(contactCount);
				std::transform(contactPoints, contactPoints + contactCount, std::back_inserter(contact.contactPoints), PxContactPointToContactPoint);

				_scene._onContactUpdate(contact);
			}
			else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
				_scene._onContactExit(contact);
			}
		}
	}

	void PhysXEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count) {
		for (PxU32 i = 0; i < count; i++) {
			const PxActor* actor0 = pairs[i].triggerActor;
			const PxActor* actor1 = pairs[i].otherActor;

			if (!actor0 || !actor1) {
				continue;
			}

			PhysicsActor* physXActor0 = static_cast<PhysicsActor*>(actor0->userData);
			PhysicsActor* physXActor1 = static_cast<PhysicsActor*>(actor1->userData);

			if (!physXActor0 || !physXActor1) {
				continue;
			}

			PhysicsShape* shape0 = static_cast<PhysicsShape*>(pairs[i].triggerShape->userData);
			PhysicsShape* shape1 = static_cast<PhysicsShape*>(pairs[i].otherShape->userData);

			PhysicsTrigger trigger;
			trigger.actor = physXActor0;
			trigger.shape = shape0;
			trigger.otherActor = physXActor1;
			trigger.otherShape = shape1;

			if (pairs[i].status & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
				_scene._onTriggerEnter(trigger);
			}
			else if (pairs[i].status & PxPairFlag::eNOTIFY_TOUCH_LOST) {
				_scene._onTriggerExit(trigger);
			}
		}
	}

	PhysXScene::PhysXScene(PhysXContext& context, const Descriptor& desc)
		: _context(context)
		, _eventCallback(*this)
	{
		PxSceneDesc sceneDesc(_context.GetPhysics().getTolerancesScale());
		sceneDesc.gravity = Vec3ToPxVec3(desc.gravity);
		sceneDesc.cpuDispatcher = &_context.GetCpuDispatcher();
		sceneDesc.filterShader = CustomFilterShader;
		sceneDesc.simulationEventCallback = &_eventCallback;

		_scene = _context.GetPhysics().createScene(sceneDesc);
		if (!_scene) {
			Log::Error("Failed to create PhysX scene.");
			return;
		}
	}

	PxFilterFlags PhysXScene::CustomFilterShader(
		PxFilterObjectAttributes attributes0, PxFilterData filterData0,
		PxFilterObjectAttributes attributes1, PxFilterData filterData1,
		PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize) 
	{
		pairFlags |= PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eTRIGGER_DEFAULT | PxPairFlag::eNOTIFY_CONTACT_POINTS | PxPairFlag::eNOTIFY_TOUCH_PERSISTS;

		return PxFilterFlag::eDEFAULT;
	}

	PhysXScene::~PhysXScene() {
		if (_scene) {
			_scene->release();
		}
	}

	void PhysXScene::JoinActor(Ref<PhysicsActor> actor) {
		if (auto dynamicActor = std::dynamic_pointer_cast<PhysXActorDynamic>(actor)) {
			_scene->addActor(*dynamicActor->GetPxRigidBody());
		}
		else if (auto staticActor = std::dynamic_pointer_cast<PhysXActorStatic>(actor)) {
			_scene->addActor(*staticActor->GetPxRigidBody());
		}
		else {
			Log::Error("Unsupported actor type for PhysX scene.");
			return;
		}
	}

	void PhysXScene::LeaveActor(Ref<PhysicsActor> actor) {
		if (auto dynamicActor = std::dynamic_pointer_cast<PhysXActorDynamic>(actor)) {
			_scene->removeActor(*dynamicActor->GetPxRigidBody());
		}
		else if (auto staticActor = std::dynamic_pointer_cast<PhysXActorStatic>(actor)) {
			_scene->removeActor(*staticActor->GetPxRigidBody());
		}
		else {
			Log::Error("Unsupported actor type for PhysX scene.");
			return;
		}
	}

	void PhysXScene::SetGravity(const vec3& gravity) {
		_scene->setGravity(Vec3ToPxVec3(gravity));
	}

	void PhysXScene::Update(float deltaTime, uint32_t steps) {
		for (uint32_t i = 0; i < steps; ++i) {
			_scene->simulate(deltaTime);
			_scene->fetchResults(true);
		}
	}

	bool PhysXScene::Raycast(const Ray& ray, RayHit& hit) {
		PxVec3 origin = Vec3ToPxVec3(ray.origin);
		PxVec3 direction = Vec3ToPxVec3(ray.direction);

		PxRaycastBuffer hitBuffer;
		if (_scene->raycast(origin, direction, ray.length, hitBuffer)) {
			hit.position = PxVec3ToVec3(hitBuffer.block.position);
			hit.normal = PxVec3ToVec3(hitBuffer.block.normal);
			hit.distance = hitBuffer.block.distance;
			return true;
		}

		return false;
	}
}