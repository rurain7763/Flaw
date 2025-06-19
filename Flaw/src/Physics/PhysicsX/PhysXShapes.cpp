#include "pch.h"
#include "PhysXShapes.h"
#include "PhysXContext.h"
#include "Helper.h"
#include "Log/Log.h"

namespace flaw {
	PhysXBoxShape::PhysXBoxShape(PhysXContext& context, const Descriptor& desc)
		: _context(context)
	{
		_material = _context.GetPhysics().createMaterial(desc.staticFriction, desc.dynamicFriction, desc.restitution);
		if (!_material) {
			Log::Error("Failed to create PhysX material.");
			return;
		}

		PxVec3 halfExtents = PxVec3(desc.size.x * 0.5f, desc.size.y * 0.5f, desc.size.z * 0.5f);
		PxBoxGeometry boxGeometry(halfExtents.x, halfExtents.y, halfExtents.z);
		_shape = _context.GetPhysics().createShape(boxGeometry, *_material);
		if (!_shape) {
			Log::Error("Failed to create PhysX shape.");
			return;
		}

		PxTransform transform(Vec3ToPxVec3(desc.offset));
		_shape->setLocalPose(transform);
		_shape->userData = this;
	}

	PhysXBoxShape::~PhysXBoxShape() {
		_shape->release();
		_material->release();
	}

	bool PhysXBoxShape::SetOffset(const vec3& offset) {
		PxVec3 currentPos = _shape->getLocalPose().p;
		if (EpsilonEqual(currentPos.x, offset.x) && EpsilonEqual(currentPos.y, offset.y) && EpsilonEqual(currentPos.z, offset.z)) {
			return false;
		}

		PxTransform transform(Vec3ToPxVec3(offset));
		_shape->setLocalPose(transform);
		return true;
	}

	bool PhysXBoxShape::SetSize(const vec3& size) {
		PxVec3 halfExtents = PxVec3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);

		const PxBoxGeometry& geometry = reinterpret_cast<const PxBoxGeometry&>(_shape->getGeometry());
		if (EpsilonEqual(geometry.halfExtents.x, halfExtents.x) && EpsilonEqual(geometry.halfExtents.y, halfExtents.y) && EpsilonEqual(geometry.halfExtents.z, halfExtents.z)) {
			return false;
		}

		_shape->setGeometry(PxBoxGeometry(halfExtents.x, halfExtents.y, halfExtents.z));
		return true;
	}

	PhysXSphereShape::PhysXSphereShape(PhysXContext& context, const Descriptor& desc)
		: _context(context)
	{
		_material = _context.GetPhysics().createMaterial(desc.staticFriction, desc.dynamicFriction, desc.restitution);
		if (!_material) {
			Log::Error("Failed to create PhysX material.");
			return;
		}

		PxSphereGeometry sphereGeometry(desc.radius);
		_shape = _context.GetPhysics().createShape(sphereGeometry, *_material);
		if (!_shape) {
			Log::Error("Failed to create PhysX shape.");
			return;
		}

		PxTransform transform(Vec3ToPxVec3(desc.offset));
		_shape->setLocalPose(transform);
		_shape->userData = this;
	}

	PhysXSphereShape::~PhysXSphereShape() {
		_shape->release();
		_material->release();
	}

	bool PhysXSphereShape::SetOffset(const vec3& offset) {
		PxVec3 currentPos = _shape->getLocalPose().p;
		if (EpsilonEqual(currentPos.x, offset.x) && EpsilonEqual(currentPos.y, offset.y) && EpsilonEqual(currentPos.z, offset.z)) {
			return false;
		}

		PxTransform transform(Vec3ToPxVec3(offset));
		_shape->setLocalPose(transform);
		return true;
	}

	bool PhysXSphereShape::SetRadius(float radius) {
		const PxSphereGeometry& geometry = reinterpret_cast<const PxSphereGeometry&>(_shape->getGeometry());
		if (EpsilonEqual(geometry.radius, radius)) {
			return false;
		}

		_shape->setGeometry(PxSphereGeometry(radius));
		return true;
	}

	PhysXMeshShape::PhysXMeshShape(PhysXContext& context, const Descriptor& desc)
		: _context(context)
	{
		_material = _context.GetPhysics().createMaterial(desc.staticFriction, desc.dynamicFriction, desc.restitution);
		if (!_material) {
			Log::Error("Failed to create PhysX material.");
			return;
		}

		PxTriangleMeshGeometry meshGeometry;
		CookMeshGeometry(desc.vertices, desc.indices, meshGeometry);

		_shape = _context.GetPhysics().createShape(meshGeometry, *_material);
		if (!_shape) {
			Log::Error("Failed to create PhysX shape.");
			return;
		}

		PxTransform transform(PxIdentity);
		_shape->setLocalPose(transform);
		_shape->userData = this;
	}

	PhysXMeshShape::~PhysXMeshShape() {
		_shape->release();
		_material->release();
	}

	bool PhysXMeshShape::SetMesh(const std::vector<vec3>& vertices, const std::vector<uint32_t>& indices) {
		PxTriangleMeshGeometry meshGeometry;
		CookMeshGeometry(vertices, indices, meshGeometry);
		_shape->setGeometry(meshGeometry);
		return true;
	}

	void PhysXMeshShape::CookMeshGeometry(const std::vector<vec3>& vertices, const std::vector<uint32_t>& indices, PxTriangleMeshGeometry& geometry) {
		std::vector<PxVec3> physXVertices;
		std::transform(vertices.begin(), vertices.end(), std::back_inserter(physXVertices), [](const vec3& v) { return Vec3ToPxVec3(v); });

		PxTriangleMeshDesc meshDesc;
		meshDesc.points.data = physXVertices.data();
		meshDesc.points.stride = sizeof(PxVec3);
		meshDesc.points.count = physXVertices.size();
		meshDesc.triangles.data = indices.data();
		meshDesc.triangles.stride = sizeof(uint32_t) * 3;
		meshDesc.triangles.count = indices.size() / 3;

		PxDefaultMemoryOutputStream writeBuffer;
		PxCookingParams cookingParams(_context.GetPhysics().getTolerancesScale());
		cookingParams.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;

		if (!PxCookTriangleMesh(cookingParams, meshDesc, writeBuffer)) {
			throw std::runtime_error("Failed to cook triangle mesh.");
		}

		PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
		PxTriangleMesh* triangleMesh = _context.GetPhysics().createTriangleMesh(readBuffer);
		if (!triangleMesh) {
			throw std::runtime_error("Failed to create triangle mesh from cooked data.");
		}

		geometry.triangleMesh = triangleMesh;
		geometry.scale = PxMeshScale(PxVec3(1.0f)); // TODO: handle scale properly
	}
}