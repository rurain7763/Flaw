#pragma once

#include "Core.h"
#include "Physics/Physics.h"

namespace flaw {
	class Physics {
	public:
		static void Init();
		static void Cleanup();

		static void Update();

		static PhysicsActor* CreateActor(const ActorDescription& desc);
		static void DestroyActor(PhysicsActor* actor);

		static bool Raycast(const Ray& ray, RayHit& hit);
	};
}