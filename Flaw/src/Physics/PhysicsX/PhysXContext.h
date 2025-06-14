#pragma once

#include "Physics/PhysicsContext.h"

#include <map>
#include <physx/PxPhysicsAPI.h>

using namespace physx;

namespace flaw {
	class PhysXContext;

	struct PhysXActor : public PhysicsActor {
		uint32_t id;
		PxShape* shape = nullptr;
		PxMaterial* material = nullptr;
		PxRigidActor* rigidActor = nullptr;

		void GetTransform(vec3& position, vec3& rotation) const override;
	};

	class PhysXEventCallback : public PxSimulationEventCallback {
	public:
		PhysXEventCallback(PhysXContext& context) : _context(context) {}
		virtual ~PhysXEventCallback() = default;

		void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
		void onTrigger(PxTriggerPair* pairs, physx::PxU32 count) override;
		void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override {}
		void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override {}
		void onWake(PxActor** actors, PxU32 count) override {}
		void onSleep(PxActor** actors, PxU32 count) override {}

	private:
		PhysXContext& _context;
	};

	class PhysXContext : public PhysicsContext {
	public:
		PhysXContext();
		virtual ~PhysXContext();

		void Update(float deltaTime, uint32_t steps = 1) override;

		PhysicsActor* CreateActor(const ActorDescription& desc) override;
		void DestroyActor(PhysicsActor* actor) override;

		bool Raycast(const Ray& ray, RayHit& hit) override;

	private:
		static PxFilterFlags CustomFilterShader(
			PxFilterObjectAttributes attributes0, PxFilterData filterData0,
			PxFilterObjectAttributes attributes1, PxFilterData filterData1,
			PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize
		);

		void CookMeshGeometry(const std::vector<vec3>& vertices, const std::vector<uint32_t>& indices, PxTriangleMeshGeometry& geometry);

		uint32_t GenerateID();
		void ReleaseID(uint32_t id);

	private:
		PxDefaultErrorCallback _errorCallback;
		PxDefaultAllocator _allocatorCallback;
		PxDefaultCpuDispatcher* _cpuDispatcher;
		PxFoundation* _foundation;
		PxPhysics* _physics;
		PxScene* _scene;

		PhysXEventCallback _eventCallback;

		uint32_t _idCounter = 0;
		std::vector<uint32_t> _freeIDs;
		std::map<uint32_t, Ref<PhysXActor>> _actors;
		std::vector<Ref<PhysicsActor>> _actorToDestroy;
	};
}