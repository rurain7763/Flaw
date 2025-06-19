#include "pch.h"
#include "Physics.h"
#include "Physics/PhysicsX/PhysXContext.h"
#include "Log/Log.h"
#include "Time/Time.h"

namespace flaw {
	static Scope<PhysicsContext> g_physicsContext;

	void Physics::Init() {
		g_physicsContext = CreateScope<PhysXContext>();
		
		Log::Info("Physics initialized with PhysX context.");
	}

	void Physics::Cleanup() {
		g_physicsContext.reset();
	}

	Ref<PhysicsActorStatic> Physics::CreateActorStatic(const PhysicsActorStatic::Descriptor& desc) {
		return g_physicsContext->CreateActorStatic(desc);
	}

	Ref<PhysicsActorDynamic> Physics::CreateActorDynamic(const PhysicsActorDynamic::Descriptor& desc) {
		return g_physicsContext->CreateActorDynamic(desc);
	}

	Ref<PhysicsBoxShape> Physics::CreateBoxShape(const PhysicsBoxShape::Descriptor& desc) {
		return g_physicsContext->CreateBoxShape(desc);
	}

	Ref<PhysicsSphereShape> Physics::CreateSphereShape(const PhysicsSphereShape::Descriptor& desc) {
		return g_physicsContext->CreateSphereShape(desc);
	}

	Ref<PhysicsMeshShape> Physics::CreateMeshShape(const PhysicsMeshShape::Descriptor& desc) {
		return g_physicsContext->CreateMeshShape(desc);
	}

	Ref<PhysicsScene> Physics::CreateScene(const PhysicsScene::Descriptor& desc) {
		return g_physicsContext->CreateScene(desc);
	}
}