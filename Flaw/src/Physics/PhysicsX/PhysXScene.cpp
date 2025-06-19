#include "pch.h"
#include "PhysXScene.h"
#include "PhysXContext.h"
#include "Helper.h"
#include "Log/Log.h"
#include "PhysXActors.h"

namespace flaw {
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

			if (pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
				std::vector<PxContactPairPoint> contactPoints(pair.contactCount);
				pair.extractContacts(contactPoints.data(), pair.contactCount);

				for (const auto& contact : contactPoints) {
					ContactPoint contactPoint = PxContactPointToContactPoint(contact);
					physXActor0->CallOnContact(physXActor1, shape1, contactPoint);
					physXActor1->CallOnContact(physXActor0, shape0, contactPoint);
				}
			}
			else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
				physXActor0->CallOnContactExit(physXActor1, shape1);
				physXActor1->CallOnContactExit(physXActor0, shape0);
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

			// TODO:
		}
	}

	PhysXScene::PhysXScene(PhysXContext& context, const Descriptor& desc)
		: _context(context)
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
		pairFlags |= PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eTRIGGER_DEFAULT | PxPairFlag::eNOTIFY_CONTACT_POINTS;

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