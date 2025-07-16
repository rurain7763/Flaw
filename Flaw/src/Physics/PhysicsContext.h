#pragma once

#include "Core.h"
#include "PhysicsTypes.h"
#include "Math/Math.h"
#include "Utils/Raycast.h"

namespace flaw {
	class PhysicsShape {
	public:
		virtual ~PhysicsShape() = default;

		virtual PhysicsShapeType GetShapeType() const = 0;
	};

	class PhysicsBoxShape : public PhysicsShape {
	public:
		struct Descriptor {
			float staticFriction;
			float dynamicFriction;
			float restitution;

			vec3 offset;
			vec3 size;
		};

		virtual ~PhysicsBoxShape() = default;

		virtual bool SetOffset(const vec3& offset) = 0;
		virtual bool SetSize(const vec3& size) = 0;

		PhysicsShapeType GetShapeType() const override {
			return PhysicsShapeType::Box;
		}
	};

	class PhysicsSphereShape : public PhysicsShape {
	public:
		struct Descriptor {
			float staticFriction;
			float dynamicFriction;
			float restitution;

			vec3 offset;
			float radius;
		};

		virtual ~PhysicsSphereShape() = default;

		virtual bool SetOffset(const vec3& offset) = 0;
		virtual bool SetRadius(float radius) = 0;

		PhysicsShapeType GetShapeType() const override {
			return PhysicsShapeType::Sphere;
		}
	};

	struct PhysicsMeshShape : public PhysicsShape {
	public:
		struct Descriptor {
			float staticFriction;
			float dynamicFriction;
			float restitution;

			std::vector<vec3> vertices;
			std::vector<uint32_t> indices;
		};

		virtual ~PhysicsMeshShape() = default;

		virtual bool SetMesh(const std::vector<vec3>& vertices, const std::vector<uint32_t>& indices) = 0;

		PhysicsShapeType GetShapeType() const override {
			return PhysicsShapeType::Mesh;
		}
	};

	class PhysicsActor {
	public:
		virtual ~PhysicsActor() = default;

		virtual bool IsJoined() const = 0;

		virtual void AttachShape(Ref<PhysicsShape> shape) = 0;
		virtual void DetachShape(Ref<PhysicsShape> shape) = 0;
		virtual bool HasShapes() const = 0;

		virtual void GetTransform(vec3& position, vec3& rotation) const = 0;

		virtual PhysicsBodyType GetBodyType() const = 0;

		void* GetUserData() const {
			return _userData;
		}

	protected:
		void* _userData;
	};

	class PhysicsActorStatic : public PhysicsActor {
	public:
		struct Descriptor {
			vec3 position;
			vec3 rotation;

			void* userData = nullptr;
		};

		virtual ~PhysicsActorStatic() = default;

		PhysicsBodyType GetBodyType() const override {
			return PhysicsBodyType::Static;
		}
	};

	class PhysicsActorDynamic : public PhysicsActor {
	public:
		struct Descriptor {
			vec3 position;
			vec3 rotation;

			float density = 1.0f;

			void* userData = nullptr;
		};

		virtual ~PhysicsActorDynamic() = default;

		virtual void UpdateMassAndInertia() = 0;

		virtual bool SetMass(float mass) = 0;

		virtual void SetKinematicState(bool isKinematic) = 0;
		virtual void SetKinematicTarget(const vec3& targetPosition, const vec3& targetRotation) = 0;
		virtual bool IsKinematic() const = 0;

		PhysicsBodyType GetBodyType() const override {
			return PhysicsBodyType::Dynamic;
		}
	};

	struct PhysicsContact {
		PhysicsActor* actor; // The actor that was involved in the contact
		PhysicsShape* shape; // The shape that was involved in the contact

		PhysicsActor* otherActor; // The other actor involved in the contact
		PhysicsShape* otherShape; // The shape of the other actor involved in the contact

		std::vector<ContactPoint> contactPoints; // The contact points of the contact
	};

	struct PhysicsTrigger {
		PhysicsActor* actor; // The actor that was involved in the trigger
		PhysicsShape* shape; // The shape that was involved in the trigger

		PhysicsActor* otherActor; // The other actor involved in the trigger
		PhysicsShape* otherShape; // The shape of the other actor involved in the trigger
	};

	class PhysicsScene {
	public:
		struct Descriptor {
			vec3 gravity = vec3(0.0f, -9.81f, 0.0f);
		};

		virtual ~PhysicsScene() = default;

		virtual void JoinActor(Ref<PhysicsActor> actor) = 0;
		virtual void LeaveActor(Ref<PhysicsActor> actor) = 0;
		
		virtual void Update(float deltaTime, uint32_t steps = 1) = 0;

		virtual void SetGravity(const vec3& gravity) = 0;

		virtual bool Raycast(const Ray& ray, RayHit& hit) = 0;

		void SetOnContactEnter(const std::function<void(PhysicsContact&)>& callback) {
			_onContactEnter = callback;
		}

		void SetOnContactUpdate(const std::function<void(PhysicsContact&)>& callback) {
			_onContactUpdate = callback;
		}

		void SetOnContactExit(const std::function<void(PhysicsContact&)>& callback) {
			_onContactExit = callback;
		}

		void SetOnTriggerEnter(const std::function<void(PhysicsTrigger&)>& callback) {
			_onTriggerEnter = callback;
		}

		void SetOnTriggerExit(const std::function<void(PhysicsTrigger&)>& callback) {
			_onTriggerExit = callback;
		}

	protected:
		std::function<void(PhysicsContact&)> _onContactEnter;
		std::function<void(PhysicsContact&)> _onContactUpdate;
		std::function<void(PhysicsContact&)> _onContactExit;

		std::function<void(PhysicsTrigger&)> _onTriggerEnter;
		std::function<void(PhysicsTrigger&)> _onTriggerExit;
	};

	class PhysicsContext {
	public:
		PhysicsContext() = default;
		virtual ~PhysicsContext() = default;

		virtual Ref<PhysicsActorStatic> CreateActorStatic(const PhysicsActorStatic::Descriptor& desc) = 0;
		virtual Ref<PhysicsActorDynamic> CreateActorDynamic(const PhysicsActorDynamic::Descriptor& desc) = 0;

		virtual Ref<PhysicsBoxShape> CreateBoxShape(const PhysicsBoxShape::Descriptor& desc) = 0;
		virtual Ref<PhysicsSphereShape> CreateSphereShape(const PhysicsSphereShape::Descriptor& desc) = 0;
		virtual Ref<PhysicsMeshShape> CreateMeshShape(const PhysicsMeshShape::Descriptor& desc) = 0;

		virtual Ref<PhysicsScene> CreateScene(const PhysicsScene::Descriptor& desc) = 0;
	};
}