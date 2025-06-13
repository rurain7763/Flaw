#pragma once

#include "Core.h"
#include "Math/Math.h"

#include <physx/PxPhysicsAPI.h>

using namespace physx;

namespace flaw {
	vec3 PxVec3ToVec3(const PxVec3& vec) {
		return vec3(vec.x, vec.y, vec.z);
	}

	PxVec3 Vec3ToPxVec3(const vec3& vec) {
		return physx::PxVec3(vec.x, vec.y, vec.z);
	}
}