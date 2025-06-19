#pragma once

#include "Core.h"
#include "Physics/PhysicsContext.h"

#include <physx/PxPhysicsAPI.h>

using namespace physx;

namespace flaw {
	class PhysXContext;
	
	class PhysXBoxShape : public PhysicsBoxShape {
	public:
		PhysXBoxShape(PhysXContext& context, const Descriptor& desc);
		~PhysXBoxShape();
		
		bool SetOffset(const vec3& offset) override;
		bool SetSize(const vec3& size) override;

		PxShape* GetPxShape() const { return _shape; }
		PxMaterial* GetPxMaterial() const { return _material; }

	private:
		PhysXContext& _context;

		PxShape* _shape;
		PxMaterial* _material = nullptr;
	};

	class PhysXSphereShape : public PhysicsSphereShape {
	public:
		PhysXSphereShape(PhysXContext& context, const Descriptor& desc);
		~PhysXSphereShape();

		bool SetOffset(const vec3& offset) override;
		bool SetRadius(float radius) override;

		PxShape* GetPxShape() const { return _shape; }
		PxMaterial* GetPxMaterial() const { return _material; }

	private:
		PhysXContext& _context;

		PxShape* _shape;
		PxMaterial* _material = nullptr;
	};

	class PhysXMeshShape : public PhysicsMeshShape {
	public:
		PhysXMeshShape(PhysXContext& context, const Descriptor& desc);
		~PhysXMeshShape();

		bool SetMesh(const std::vector<vec3>& vertices, const std::vector<uint32_t>& indices) override;

		PxShape* GetPxShape() const { return _shape; }
		PxMaterial* GetPxMaterial() const { return _material; }

	private:
		void CookMeshGeometry(const std::vector<vec3>& vertices, const std::vector<uint32_t>& indices, PxTriangleMeshGeometry& geometry);

	private:
		PhysXContext& _context;

		PxShape* _shape;
		PxMaterial* _material = nullptr;
	};
}