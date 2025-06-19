#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Physics/PhysicsTypes.h"
#include "PhysXShapes.h"

#include <physx/PxPhysicsAPI.h>

using namespace physx;

namespace flaw {
	inline vec3 PxVec3ToVec3(const PxVec3& vec) {
		return vec3(vec.x, vec.y, vec.z);
	}

	inline PxVec3 Vec3ToPxVec3(const vec3& vec) {
		return physx::PxVec3(vec.x, vec.y, vec.z);
	}

	inline PxShape* GetPxShapeFromShape(Ref<PhysicsShape> shape) {
		if (auto boxShape = std::dynamic_pointer_cast<PhysXBoxShape>(shape)) {
			return boxShape->GetPxShape();
		}
		else if (auto sphereShape = std::dynamic_pointer_cast<PhysXSphereShape>(shape)) {
			return sphereShape->GetPxShape();
		}
		else if (auto meshShape = std::dynamic_pointer_cast<PhysXMeshShape>(shape)) {
			return meshShape->GetPxShape();
		}

		return nullptr;
	}

	inline ContactPoint PxContactPointToContactPoint(const PxContactPairPoint& contact) {
		ContactPoint cp;
		cp.position = PxVec3ToVec3(contact.position);
		cp.normal = PxVec3ToVec3(contact.normal);
		cp.impulse = PxVec3ToVec3(contact.impulse);
		return cp;
	}
}