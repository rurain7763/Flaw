#include "pch.h"
#include "PhysXContext.h"
#include "Log/Log.h"
#include "Helper.h"
#include "PhysXActors.h"
#include "PhysXShapes.h"
#include "PhysXScene.h"

namespace flaw {
	PhysXContext::PhysXContext() {
		_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, _allocatorCallback, _errorCallback);
		if (!_foundation) {
			Log::Error("Failed to create PhysX foundation.");
			return;
		}

		PxTolerancesScale scale;
		scale.length = 1.0f;
		scale.speed = 10.0f;
		_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation, scale);
		if (!_physics) {
			_foundation->release();
			Log::Error("Failed to create PhysX physics.");
			return;
		}

		_cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
		if (!_cpuDispatcher) {
			_physics->release();
			_foundation->release();
			Log::Error("Failed to create PhysX CPU dispatcher.");
			return;
		}
	}

	PhysXContext::~PhysXContext() {
		_physics->release();
		_cpuDispatcher->release();
		_foundation->release();
	}

	Ref<PhysicsActorStatic> PhysXContext::CreateActorStatic(const PhysicsActorStatic::Descriptor& desc) {
		return CreateRef<PhysXActorStatic>(*this, desc);
	}

	Ref<PhysicsActorDynamic> PhysXContext::CreateActorDynamic(const PhysicsActorDynamic::Descriptor& desc) {
		return CreateRef<PhysXActorDynamic>(*this, desc);
	}

	Ref<PhysicsBoxShape> PhysXContext::CreateBoxShape(const PhysicsBoxShape::Descriptor& desc) {
		return CreateRef<PhysXBoxShape>(*this, desc);
	}

	Ref<PhysicsSphereShape> PhysXContext::CreateSphereShape(const PhysicsSphereShape::Descriptor& desc) {
		return CreateRef<PhysXSphereShape>(*this, desc);
	}

	Ref<PhysicsMeshShape> PhysXContext::CreateMeshShape(const PhysicsMeshShape::Descriptor& desc) {
		return CreateRef<PhysXMeshShape>(*this, desc);
	}

	Ref<PhysicsScene> PhysXContext::CreateScene(const PhysicsScene::Descriptor& desc) {
		return CreateRef<PhysXScene>(*this, desc);
	}
}