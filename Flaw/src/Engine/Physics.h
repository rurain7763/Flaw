#pragma once

#include "Core.h"
#include "Physics/Physics.h"

namespace flaw {
	class Physics {
	public:
		static void Init();
		static void Cleanup();

		static Ref<PhysicsActorStatic> CreateActorStatic(const PhysicsActorStatic::Descriptor& desc);
		static Ref<PhysicsActorDynamic> CreateActorDynamic(const PhysicsActorDynamic::Descriptor& desc);

		static Ref<PhysicsBoxShape> CreateBoxShape(const PhysicsBoxShape::Descriptor& desc);
		static Ref<PhysicsSphereShape> CreateSphereShape(const PhysicsSphereShape::Descriptor& desc);
		static Ref<PhysicsMeshShape> CreateMeshShape(const PhysicsMeshShape::Descriptor& desc);

		static Ref<PhysicsScene> CreateScene(const PhysicsScene::Descriptor& desc);
	};
}