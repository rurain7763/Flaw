#pragma once

#include "Core.h"
#include "Physics/Physics.h"
#include "Entity.h"

namespace flaw {
	struct CollisionInfo {
		Entity entity0;
		PhysicsShapeType shapeType0;
		Entity entity1;
		PhysicsShapeType shapeType1;
		std::vector<ContactPoint> contactPoints;
	};

	struct TriggerInfo {
		Entity entity0;
		PhysicsShapeType shapeType0;
		Entity entity1;
		PhysicsShapeType shapeType1;
	};

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