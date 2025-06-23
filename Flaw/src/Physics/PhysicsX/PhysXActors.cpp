#include "pch.h"
#include "PhysXActors.h"
#include "PhysXContext.h"
#include "PhysXShapes.h"
#include "Helper.h"
#include "Log/Log.h"

namespace flaw {
	PhysXActorStatic::PhysXActorStatic(PhysXContext& context, const Descriptor& desc)
		: _context(context)
	{
		PxTransform transform(Vec3ToPxVec3(desc.position));
		glm::quat rotation = glm::quat(desc.rotation);
		transform.q = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);

		_rigidBody = _context.GetPhysics().createRigidStatic(transform);
		if (!_rigidBody) {
			Log::Error("Failed to create PhysX rigid static actor.");
			return;
		}

		_rigidBody->userData = this;
		_userData = desc.userData;
	}

	PhysXActorStatic::~PhysXActorStatic() {
		_rigidBody->release();
	}

	bool PhysXActorStatic::IsJoined() const {
		return _rigidBody->getScene() != nullptr;
	}

	void PhysXActorStatic::AttatchShape(Ref<PhysicsShape> shape) {
		auto it = _shapes.find(shape);
		if (it != _shapes.end()) {
			return;
		}

		PxShape* pxShape = GetPxShapeFromShape(shape);
		if (!pxShape) {
			return;
		}

		_rigidBody->attachShape(*pxShape);
		_shapes.insert(shape);
	}

	void PhysXActorStatic::DetachShape(Ref<PhysicsShape> shape) {
		auto it = _shapes.find(shape);
		if (it == _shapes.end()) {
			return;
		}

		PxShape* pxShape = GetPxShapeFromShape(shape);
		if (!pxShape) {
			return;
		}

		_rigidBody->detachShape(*pxShape);
		_shapes.erase(it);
	}

	bool PhysXActorStatic::HasShapes() const {
		return !_shapes.empty();
	}

	void PhysXActorStatic::GetTransform(vec3& position, vec3& rotation) const {
		PxTransform transform = _rigidBody->getGlobalPose();
		position = PxVec3ToVec3(transform.p);
		rotation = glm::eulerAngles(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
	}

	PhysXActorDynamic::PhysXActorDynamic(PhysXContext& context, const Descriptor& desc)
		: _context(context)
	{
		PxTransform transform(Vec3ToPxVec3(desc.position));
		glm::quat rotation = glm::quat(desc.rotation);
		transform.q = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);

		_rigidBody = _context.GetPhysics().createRigidDynamic(transform);
		if (!_rigidBody) {
			Log::Error("Failed to create PhysX rigid dynamic actor.");
			return;
		}

		_rigidBody->userData = this;
		_userData = desc.userData;

		SetMass(desc.density);
	}

	PhysXActorDynamic::~PhysXActorDynamic() {
		_rigidBody->release();
	}

	bool PhysXActorDynamic::IsJoined() const {
		return _rigidBody->getScene() != nullptr;
	}

	void PhysXActorDynamic::AttatchShape(Ref<PhysicsShape> shape) {
		auto it = _shapes.find(shape);
		if (it != _shapes.end()) {
			return;
		}

		PxShape* pxShape = GetPxShapeFromShape(shape);
		if (!pxShape) {
			Log::Error("Failed to create PhysX shape.");
			return;
		}

		_rigidBody->attachShape(*pxShape);
		_shapes.insert(shape);
	}

	void PhysXActorDynamic::DetachShape(Ref<PhysicsShape> shape) {
		auto it = _shapes.find(shape);
		if (it == _shapes.end()) {
			return;
		}

		PxShape* pxShape = GetPxShapeFromShape(shape);
		if (!pxShape) {
			return;
		}

		_rigidBody->detachShape(*pxShape);
		_shapes.erase(it);
	}

	bool PhysXActorDynamic::HasShapes() const {
		return !_shapes.empty();
	}

	void PhysXActorDynamic::UpdateMassAndInertia() {
		PxRigidBodyExt::updateMassAndInertia(*_rigidBody, _rigidBody->getMass());
	}

	void PhysXActorDynamic::GetTransform(vec3& position, vec3& rotation) const {
		PxTransform transform = _rigidBody->getGlobalPose();
		position = PxVec3ToVec3(transform.p);
		rotation = glm::eulerAngles(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
	}

	bool PhysXActorDynamic::SetMass(float mass) {
		if (EpsilonEqual(_rigidBody->getMass(), mass)) {
			return false;
		}

		_rigidBody->setMass(mass);
		return true;
	}

	void PhysXActorDynamic::SetKinematicState(bool isKinematic) {
		_rigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, isKinematic);
	}

	void PhysXActorDynamic::SetKinematicTarget(const vec3& targetPosition, const vec3& targetRotation) {
		PxTransform transform(Vec3ToPxVec3(targetPosition));
		glm::quat rotation = glm::quat(targetRotation);
		transform.q = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);
		_rigidBody->setKinematicTarget(transform);
	}

	bool PhysXActorDynamic::IsKinematic() const {
		return _rigidBody->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC;
	}
}