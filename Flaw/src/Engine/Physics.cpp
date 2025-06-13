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

	void Physics::Update() {
		g_physicsContext->Update(Time::DeltaTime());
	}

	PhysicsActor* Physics::CreateActor(const ActorDescription& desc) {
		return g_physicsContext->CreateActor(desc);
	}

	void Physics::DestroyActor(PhysicsActor* actor) {
		g_physicsContext->DestroyActor(actor);
	}
}