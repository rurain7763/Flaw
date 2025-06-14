#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Utils/Raycast.h"

namespace flaw {
	struct PhysicsActor {
		void* userData;

		std::function<void(PhysicsActor*)> onContact;
		std::function<void(PhysicsActor*)> onTrigger;

		virtual ~PhysicsActor() = default;

		virtual void GetTransform(vec3& position, vec3& rotation) const = 0;
	};

	enum class PhysicsBodyType {
		Static,
		Dynamic,
		Kinematic
	};

	struct RigidBody {
		PhysicsBodyType type = PhysicsBodyType::Static;

		float staticFriction;
		float dynamicFriction;
		float restitution;

		float density;
	};

	struct PhysicsCollider {
		vec3 position;
		vec3 rotation;

		virtual ~PhysicsCollider() = default;
	};

	struct PhysicsBoxCollider : public PhysicsCollider {
		vec3 size;
	};

	struct PhysicsSphereCollider : public PhysicsCollider {
		float radius;
	};

	struct PhysicsMeshCollider : public PhysicsCollider {
		std::vector<vec3> vertices;
		std::vector<uint32_t> indices;
	};

	struct ActorDescription {
		RigidBody* body;
		PhysicsCollider* collider;
		void* userData = nullptr;
	};

	class PhysicsContext {
	public:
		PhysicsContext() = default;
		virtual ~PhysicsContext() = default;

		virtual void Update(float deltaTime, uint32_t steps = 1) = 0;

		virtual PhysicsActor* CreateActor(const ActorDescription& desc) = 0;
		virtual void DestroyActor(PhysicsActor* actor) = 0;

		virtual bool Raycast(const Ray& ray, RayHit& hit) = 0;
	};
}