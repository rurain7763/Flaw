#pragma once

#include "Physics/PhysicsContext.h"

#include <physx/PxPhysicsAPI.h>

using namespace physx;

namespace flaw {
	class PhysXContext;
	class PhysXScene;

	class PhysXEventCallback : public PxSimulationEventCallback {
	public:
		PhysXEventCallback(PhysXScene& scene) : _scene(scene) {}
		virtual ~PhysXEventCallback() = default;

		void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
		void onTrigger(PxTriggerPair* pairs, physx::PxU32 count) override;
		void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override {}
		void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override {}
		void onWake(PxActor** actors, PxU32 count) override {}
		void onSleep(PxActor** actors, PxU32 count) override {}

	private:
		PhysXScene& _scene;
	};

	class PhysXScene : public PhysicsScene {
	public:
		PhysXScene(PhysXContext& context, const Descriptor& desc);
		~PhysXScene();

		void JoinActor(Ref<PhysicsActor> actor) override;
		void LeaveActor(Ref<PhysicsActor> actor) override;

		void SetGravity(const vec3& gravity) override;

		void Update(float deltaTime, uint32_t steps = 1) override;

		bool Raycast(const Ray& ray, RayHit& hit) override;

	private:
		static PxFilterFlags CustomFilterShader(
			PxFilterObjectAttributes attributes0, PxFilterData filterData0,
			PxFilterObjectAttributes attributes1, PxFilterData filterData1,
			PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize
		);

	private:
		friend class PhysXEventCallback;

		PhysXContext& _context;
		
		PxScene* _scene = nullptr;

		PhysXEventCallback _eventCallback;
	};
}