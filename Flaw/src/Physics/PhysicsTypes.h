#pragma once

#include "Core.h"
#include "Math/Math.h"

namespace flaw {
	enum class PhysicsBodyType {
		Static,
		Dynamic,
	};

	enum class PhysicsShapeType {
		Box,
		Sphere,
		Mesh,
		Count,
	};

	struct ContactPoint {
		vec3 position;
		vec3 normal;
		vec3 impulse;
	};
}