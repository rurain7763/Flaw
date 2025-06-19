#pragma once

#include "Physics/PhysicsContext.h"

#include <physx/PxPhysicsAPI.h>

using namespace physx;

namespace flaw {
	class PhysXContext;

	class PhysXContext : public PhysicsContext {
	public:
		PhysXContext();
		virtual ~PhysXContext();

		Ref<PhysicsActorStatic> CreateActorStatic(const PhysicsActorStatic::Descriptor& desc) override;
		Ref<PhysicsActorDynamic> CreateActorDynamic(const PhysicsActorDynamic::Descriptor& desc) override;

		Ref<PhysicsBoxShape> CreateBoxShape(const PhysicsBoxShape::Descriptor& desc) override;
		Ref<PhysicsSphereShape> CreateSphereShape(const PhysicsSphereShape::Descriptor& desc) override;
		Ref<PhysicsMeshShape> CreateMeshShape(const PhysicsMeshShape::Descriptor& desc) override;

		Ref<PhysicsScene> CreateScene(const PhysicsScene::Descriptor& desc) override;
		
		PxDefaultCpuDispatcher& GetCpuDispatcher() const { return *_cpuDispatcher; }
		PxPhysics& GetPhysics() const { return *_physics; }

	private:
		PxDefaultErrorCallback _errorCallback;
		PxDefaultAllocator _allocatorCallback;
		PxFoundation* _foundation;
		PxDefaultCpuDispatcher* _cpuDispatcher;
		PxPhysics* _physics;

		std::vector<Ref<PhysicsActor>> _actorToDestroy;
	};
}