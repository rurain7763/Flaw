#pragma once

#include "Core.h"
#include "Physics/PhysicsContext.h"

#include <physx/PxPhysicsAPI.h>

using namespace physx;

namespace flaw {
	class PhysXContext;
	
	class PhysXActorStatic : public PhysicsActorStatic {
	public:
		PhysXActorStatic(PhysXContext& context, const Descriptor& desc);
		~PhysXActorStatic();

		bool IsJoined() const override;

		void AttatchShape(Ref<PhysicsShape> shape) override;
		void DetachShape(Ref<PhysicsShape> shape) override;
		bool HasShapes() const override;

		void GetTransform(vec3& position, vec3& rotation) const override;

		PxRigidStatic* GetPxRigidBody() const { return _rigidBody; }

	private:
		PhysXContext& _context;

		PxRigidStatic* _rigidBody = nullptr;

		std::unordered_set<Ref<PhysicsShape>> _shapes;
	};

	class PhysXActorDynamic : public PhysicsActorDynamic {
	public:
		PhysXActorDynamic(PhysXContext& context, const Descriptor& desc);
		~PhysXActorDynamic();

		bool IsJoined() const override;

		void AttatchShape(Ref<PhysicsShape> shape) override;
		void DetachShape(Ref<PhysicsShape> shape) override;
		bool HasShapes() const override;

		void UpdateMassAndInertia() override;

		void GetTransform(vec3& position, vec3& rotation) const override;
		bool SetMass(float mass) override;

		PxRigidDynamic* GetPxRigidBody() const { return _rigidBody; }

	private:
		PhysXContext& _context;

		PxRigidDynamic* _rigidBody = nullptr;

		std::unordered_set<Ref<PhysicsShape>> _shapes;
	};
}