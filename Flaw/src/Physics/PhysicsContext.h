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

		virtual void AttatchShape(Ref<PhysicsShape> shape) = 0;
		virtual void DetachShape(Ref<PhysicsShape> shape) = 0;
		virtual bool HasShapes() const = 0;

		virtual void GetTransform(vec3& position, vec3& rotation) const = 0;

		virtual PhysicsBodyType GetBodyType() const = 0;

		void SetOnContact(const std::function<void(PhysicsActor*, PhysicsShape*, ContactPoint&)>& callback) {
			_onContact = callback;
		}

		void SetOnContactExit(const std::function<void(PhysicsActor*, PhysicsShape*)>& callback) {
			_onContactExit = callback;
		}

		void CallOnContact(PhysicsActor* other, PhysicsShape* shape, ContactPoint& contactPoint) {
			if (_onContact) {
				_onContact(this, shape, contactPoint);
			}
		}

		void CallOnContactExit(PhysicsActor* other, PhysicsShape* shape) {
			if (_onContactExit) {
				_onContactExit(this, shape);
			}
		}

	protected:
		void* _userData;

		std::function<void(PhysicsActor*, PhysicsShape*, ContactPoint&)> _onContact;
		std::function<void(PhysicsActor*, PhysicsShape*)> _onContactExit;
		std::function<void(PhysicsActor*)> _onTrigger;
		std::function<void(PhysicsActor*)> _onTriggerExit;
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

		PhysicsBodyType GetBodyType() const override {
			return PhysicsBodyType::Dynamic;
		}
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
		virtual bool Raycast(const Ray& ray, RayHit& hit) = 0;
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