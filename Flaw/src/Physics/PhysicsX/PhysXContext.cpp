#include "pch.h"
#include "PhysXContext.h"
#include "Log/Log.h"
#include "Helper.h"

namespace flaw {
	void PhysXActor::GetTransform(vec3& position, vec3& rotation) const {
		PxTransform transform = rigidActor->getGlobalPose();
		position = PxVec3ToVec3(transform.p);
		rotation = glm::eulerAngles(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
	}

	void PhysXEventCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) {
		auto actor0 = pairHeader.actors[0];
		auto actor1 = pairHeader.actors[1];
		
		if (!actor0 || !actor1) {
			return;
		}

		PhysXActor* physXActor0 = static_cast<PhysXActor*>(actor0->userData);
		PhysXActor* physXActor1 = static_cast<PhysXActor*>(actor1->userData);

		if (!physXActor0 || !physXActor1) {
			return;
		}

		for (PxU32 i = 0; i < nbPairs; i++) {
			const PxContactPair& pair = pairs[i];
		}

		if (physXActor0->onContact) {
			physXActor0->onContact(physXActor1);
		}

		if (physXActor1->onContact) {
			physXActor1->onContact(physXActor0);
		}
	}

	void PhysXEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count) {
		for (PxU32 i = 0; i < count; i++) {
			const PxActor* actor0 = pairs[i].triggerActor;
			const PxActor* actor1 = pairs[i].otherActor;

			if (!actor0 || !actor1) {
				continue;
			}

			PhysXActor* physXActor0 = static_cast<PhysXActor*>(actor0->userData);
			PhysXActor* physXActor1 = static_cast<PhysXActor*>(actor1->userData);

			if (!physXActor0 || !physXActor1) {
				continue;
			}

			if (physXActor0->onTrigger) {
				physXActor0->onTrigger(physXActor1);
			}

			if (physXActor1->onTrigger) {
				physXActor1->onTrigger(physXActor0);
			}
		}
	}

	void PhysXDeletionListener::onRelease(const PxBase* observed, void* userData, PxDeletionEventFlag::Enum deletionEvent) {
	}

	PhysXContext::PhysXContext() 
		: _eventCallback(*this)
		, _deletionListener(*this)
	{
		_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, _allocatorCallback, _errorCallback);
		if (!_foundation) {
			Log::Error("Failed to create PhysX foundation.");
			return;
		}

		PxTolerancesScale scale;
		scale.length = 1.0f;
		scale.speed = 10.0f;
		_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation, scale);
		if (!_physics) {
			_foundation->release();
			Log::Error("Failed to create PhysX physics.");
			return;
		}

		_physics->registerDeletionListener(_deletionListener, physx::PxDeletionEventFlag::eUSER_RELEASE);

		_cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
		if (!_cpuDispatcher) {
			_physics->release();
			_foundation->release();
			Log::Error("Failed to create PhysX CPU dispatcher.");
			return;
		}

		PxSceneDesc sceneDesc(_physics->getTolerancesScale());
		sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
		sceneDesc.cpuDispatcher = _cpuDispatcher;
		sceneDesc.filterShader = CustomFilterShader;
		sceneDesc.simulationEventCallback = &_eventCallback;

		_scene = _physics->createScene(sceneDesc);
		if (!_scene) {
			_cpuDispatcher->release();
			_physics->release();
			_foundation->release();
			Log::Error("Failed to create PhysX scene.");
			return;
		}
	}

	PxFilterFlags PhysXContext::CustomFilterShader(
		PxFilterObjectAttributes attributes0, PxFilterData filterData0,
		PxFilterObjectAttributes attributes1, PxFilterData filterData1,
		PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
	{
		pairFlags |= PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eTRIGGER_DEFAULT;

		return PxFilterFlag::eDEFAULT;
	}

	PhysXContext::~PhysXContext() {
		if (_scene) {
			_scene->release();
		}
		if (_physics) {
			_physics->release();
		}
		if (_cpuDispatcher) {
			_cpuDispatcher->release();
		}
		if (_foundation) {
			_foundation->release();
		}
	}

	void PhysXContext::Update(float deltaTime, uint32_t steps) {
		for (uint32_t i = 0; i < steps; ++i) {
			_scene->simulate(deltaTime);
			_scene->fetchResults(true);
		}

		_actorToDestroy.clear();
	}

	PhysicsActor* PhysXContext::CreateActor(const ActorDescription& desc) {
		PxTransform transform(Vec3ToPxVec3(desc.collider->position));
		glm::quat rotation = glm::quat(desc.collider->rotation);
		transform.q = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);

		auto actor = CreateRef<PhysXActor>();
		actor->material = _physics->createMaterial(desc.body->staticFriction, desc.body->dynamicFriction, desc.body->restitution);

		if (!actor->material) {
			Log::Error("Failed to create PhysX material.");
			return nullptr;
		}

		if (PhysicsBoxCollider* boxCollider = dynamic_cast<PhysicsBoxCollider*>(desc.collider)) {
			PxBoxGeometry boxGeometry(boxCollider->size.x * 0.5f, boxCollider->size.y * 0.5f, boxCollider->size.z * 0.5f);
			actor->shape = _physics->createShape(boxGeometry, *actor->material);
		}
		else if (PhysicsSphereCollider* sphereCollider = dynamic_cast<PhysicsSphereCollider*>(desc.collider)) {
			PxSphereGeometry sphereGeometry(sphereCollider->radius);
			actor->shape = _physics->createShape(sphereGeometry, *actor->material);
		}
		else if (PhysicsMeshCollider* meshCollider = dynamic_cast<PhysicsMeshCollider*>(desc.collider)) {
			PxTriangleMeshGeometry meshGeometry;
			CookMeshGeometry(meshCollider->vertices, meshCollider->indices, meshGeometry);
			actor->shape = _physics->createShape(meshGeometry, *actor->material);
		}
		else {
			Log::Error("Unsupported collider type.");
			return nullptr;
		}

		if (!actor->shape) {
			Log::Error("Failed to create PhysX shape.");
			return nullptr;
		}

		if (desc.body->type == PhysicsBodyType::Static) {
			actor->rigidActor = PxCreateStatic(*_physics, transform, *actor->shape);
		}
		else if (desc.body->type == PhysicsBodyType::Dynamic) {
			actor->rigidActor = PxCreateDynamic(*_physics, transform, *actor->shape, desc.body->density);
			actor->rigidActor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
		}

		if (!actor->rigidActor) {
			Log::Error("Failed to create PhysX rigid actor.");
			return nullptr;
		}

		actor->id = GenerateID();
		actor->rigidActor->userData = actor.get();
		actor->userData = desc.userData;

		_actors[actor->id] = actor;

		_scene->addActor(*actor->rigidActor);

		return actor.get();
	}

	void PhysXContext::CookMeshGeometry(const std::vector<vec3>& vertices, const std::vector<uint32_t>& indices, PxTriangleMeshGeometry& geometry) {
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
		PxCookingParams cookingParams(_physics->getTolerancesScale());
		cookingParams.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;

		if (!PxCookTriangleMesh(cookingParams, meshDesc, writeBuffer)) {
			throw std::runtime_error("Failed to cook triangle mesh.");
		}

		PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
		PxTriangleMesh* triangleMesh = _physics->createTriangleMesh(readBuffer);
		if (!triangleMesh) {
			throw std::runtime_error("Failed to create triangle mesh from cooked data.");
		}

		geometry.triangleMesh = triangleMesh;
		geometry.scale = PxMeshScale(PxVec3(1.0f)); // TODO: handle scale properly
	}

	void PhysXContext::DestroyActor(PhysicsActor* actor) {
		auto physXActor = static_cast<PhysXActor*>(actor);

		_scene->removeActor(*physXActor->rigidActor);
		
		if (physXActor->shape) {
			physXActor->shape->release();
		}

		if (physXActor->material) {
			physXActor->material->release();
		}

		if (physXActor->rigidActor) {
			physXActor->rigidActor->release();
		}

		ReleaseID(physXActor->id);

		_actorToDestroy.push_back(_actors[physXActor->id]);
		_actors.erase(physXActor->id);
	}

	uint32_t PhysXContext::GenerateID() {
		if (_freeIDs.empty()) {
			return _idCounter++;
		}
		else {
			uint32_t id = _freeIDs.back();
			_freeIDs.pop_back();
			return id;
		}
	}

	void PhysXContext::ReleaseID(uint32_t id) {
		_freeIDs.push_back(id);
	}

	bool PhysXContext::Raycast(const Ray& ray, RayHit& hit) {
		PxVec3 origin = Vec3ToPxVec3(ray.origin);
		PxVec3 direction = Vec3ToPxVec3(ray.direction);

		PxRaycastBuffer hitBuffer;
		if (_scene->raycast(origin, direction, ray.length, hitBuffer)) {
			hit.position = PxVec3ToVec3(hitBuffer.block.position);
			hit.normal = PxVec3ToVec3(hitBuffer.block.normal);
			hit.distance = hitBuffer.block.distance;
			return true;
		}

		return false;
	}
}