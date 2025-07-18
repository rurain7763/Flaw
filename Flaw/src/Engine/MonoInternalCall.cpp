#include "pch.h"
#include "MonoInternalCall.h"
#include "Time/Time.h"
#include "Scripting.h"
#include "AssetManager.h"
#include "Assets.h"
#include "Physics.h"
#include "PhysicsSystem.h"
#include "AnimationSystem.h"
#include "SkeletalSystem.h"

#include <mono/jit/jit.h>
#include <mono/metadata/reflection.h>
#include <fmt/format.h>

namespace flaw {
	void LogInfo(MonoString* text) {
		char* t = mono_string_to_utf8(text);
		Log::Info("%s", t);
		mono_free(t);
	}

	void DestroyEntity(UUID uuid) {
		Scripting::GetScene().DestroyEntityByUUID(uuid);
	}

	uint64_t FindEntityByName(MonoString* name) {
		uint64_t found = std::numeric_limits<uint64_t>::max();

		char* n = mono_string_to_utf8(name);
		Entity entity = Scripting::GetScene().FindEntityByName(n);
		if (entity) {
			found = (uint64_t)entity.GetUUID();
		}
		mono_free(n);

		return found;
	}

	uint64_t CreateEntity_Prefab(AssetHandle prefab) {
		auto prefabAsset = AssetManager::GetAsset<PrefabAsset>(prefab);
		if (!prefabAsset) {
			Log::Error("Prefab asset not found with handle: %llu", prefab);
			return UUID();
		}
		Entity entity = prefabAsset->GetPrefab()->CreateEntity(Scripting::GetScene());
		return entity.GetUUID();
	}

	uint64_t CreateEntityWithTransform_Prefab(AssetHandle prefab, vec3& position, vec3& rotation, vec3& scale) {
		auto prefabAsset = AssetManager::GetAsset<PrefabAsset>(prefab);
		if (!prefabAsset) {
			Log::Error("Prefab asset not found with handle: %llu", prefab);
			return UUID();
		}
		Entity entity = prefabAsset->GetPrefab()->CreateEntity(Scripting::GetScene(), position, rotation, scale);
		return entity.GetUUID();
	}

	MonoString* GetEntityName_Entity(UUID uuid) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		auto& comp = entity.GetComponent<EntityComponent>();
		return mono_string_new(Scripting::GetMonoScriptDomain().GetMonoDomain(), comp.name.c_str());
	}

	void GetPosition_Transform(UUID uuid, vec3& position) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		auto& comp = entity.GetComponent<TransformComponent>();
		position = comp.GetWorldPosition();
	}

	void SetPosition_Transform(UUID uuid, vec3& position) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		auto& comp = entity.GetComponent<TransformComponent>();

		vec3 parentWorldpos = vec3(0.0);
		if (entity.HasParent()) {
			parentWorldpos = entity.GetParent().GetComponent<TransformComponent>().GetWorldPosition();
		}

		comp.position = position - parentWorldpos;
		comp.dirty = true;
	}

	void GetRotation_Transform(UUID uuid, vec3& rotation) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		auto& comp = entity.GetComponent<TransformComponent>();
		rotation = comp.rotation;
	}

	void SetRotation_Transform(UUID uuid, vec3& rotation) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		auto& comp = entity.GetComponent<TransformComponent>();
		comp.rotation = rotation;
		comp.dirty = true;
	}

	void GetScale_Transform(UUID uuid, vec3& scale) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		auto& comp = entity.GetComponent<TransformComponent>();
		scale = comp.scale;
	}

	void SetScale_Transform(UUID uuid, vec3& scale) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		auto& comp = entity.GetComponent<TransformComponent>();
		comp.scale = scale;
		comp.dirty = true;
	}

	void GetForward_Transform(UUID uuid, vec3& forward) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		auto& comp = entity.GetComponent<TransformComponent>();
		forward = comp.GetWorldFront();
	}

	void GetBodyType_RigidBody2D(UUID uuid, int32_t& bodyType) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		if (entity.HasComponent<Rigidbody2DComponent>()) {
			auto& comp = entity.GetComponent<Rigidbody2DComponent>();
			bodyType = (int32_t)comp.bodyType;
		}
	}

	void SetBodyType_RigidBody2D(UUID uuid, int32_t bodyType) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		if (entity.HasComponent<Rigidbody2DComponent>()) {
			auto& comp = entity.GetComponent<Rigidbody2DComponent>();
			comp.bodyType = (Rigidbody2DComponent::BodyType)bodyType;
		}
	}

	void GetLinearVelocity_RigidBody2D(UUID uuid, vec2& velocity) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		if (entity.HasComponent<Rigidbody2DComponent>()) {
			auto& comp = entity.GetComponent<Rigidbody2DComponent>();
			velocity = comp.linearVelocity;
		}
	}

	bool GetKeyDown_Input(KeyCode key) {
		return Input::GetKeyDown(key);
	}

	bool GetKeyUp_Input(KeyCode key) {
		return Input::GetKeyUp(key);
	}

	bool GetKey_Input(KeyCode key) {
		return Input::GetKey(key);
	}

	void GetMousePosition_Input(float& x, float& y) {
		x = Input::GetMouseX();
		y = Input::GetMouseY();
	}

	bool GetMouseButtonDown_Input(MouseButton button) {
		return Input::GetMouseButtonDown(button);
	}

	bool GetMouseButtonUp_Input(MouseButton button) {
		return Input::GetMouseButtonUp(button);
	}

	bool GetMouseButton_Input(MouseButton button) {
		return Input::GetMouseButton(button);
	}

	bool Raycast_Physics(const Ray& ray, RayHit& hit) {
		return Scripting::GetScene().GetPhysicsSystem().GetPhysicsScene().Raycast(ray, hit);
	}

	void ScreenToWorld_Camera(UUID uuid, vec2& screenPos, vec3& worldPos) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		if (entity.HasComponent<CameraComponent>()) {
			auto& transformComp = entity.GetComponent<TransformComponent>();
			auto& cameraComp = entity.GetComponent<CameraComponent>();

			vec4 viewPort;
			Scripting::GetApplication().GetViewport(viewPort.x, viewPort.y, viewPort.z, viewPort.w);

			mat4 viewMatrix = ViewMatrix(transformComp.position, transformComp.rotation);
			mat4 projectionMatrix = cameraComp.GetProjectionMatrix();
			
			worldPos = ScreenToWorld(screenPos, viewPort, projectionMatrix, viewMatrix);
		}
	}

	void PlayState_Animator(UUID uuid, int32_t stateIndex) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		auto& animationSys = Scripting::GetScene().GetAnimationSystem();
		if (!animationSys.HasAnimatorJobContext(entity)) {
			return;
		}
	
		auto runtimeAnimator = animationSys.GetAnimatorJobContext(entity).runtimeAnimator;
		runtimeAnimator->PlayState(stateIndex);
	}

	void AttachEntityToSocket_SkeletalMesh(UUID uuid, UUID target, MonoString* socketName) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		auto targetEntity = Scripting::GetScene().FindEntityByUUID(target);
		FASSERT(targetEntity, "Target entity not found with UUID");

		if (entity.HasComponent<SkeletalMeshComponent>()) {
			auto& skeletalSys = Scripting::GetScene().GetSkeletalSystem();
			char* socketNameStr = mono_string_to_utf8(socketName);

			skeletalSys.AttachEntityToSocket(entity, targetEntity, socketNameStr);

			mono_free(socketNameStr);
		}
	}
}