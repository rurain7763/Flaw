#include "pch.h"
#include "MonoInternalCall.h"
#include "Time/Time.h"
#include "Scripting.h"

#include <mono/jit/jit.h>
#include <mono/metadata/reflection.h>
#include <fmt/format.h>

namespace flaw {
	void LogInfo(MonoString* text) {
		char* t = mono_string_to_utf8(text);
		Log::Info("%s", t);
		mono_free(t);
	}

	float GetDeltaTime() {
		return Time::DeltaTime();
	}

	float GetTimeSinceStart() {
		return Scripting::GetTimeSinceStart();
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

	void GetPosition_Transform(UUID uuid, vec3& position) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		if (entity.HasComponent<TransformComponent>()) {
			auto& comp = entity.GetComponent<TransformComponent>();
			position = comp.position;
		}
	}

	void SetPosition_Transform(UUID uuid, vec3& position) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		if (entity.HasComponent<TransformComponent>()) {
			auto& comp = entity.GetComponent<TransformComponent>();
			comp.dirty = true;
			comp.position = position;
		}
	}

	void GetRotation_Transform(UUID uuid, vec3& rotation) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		if (entity.HasComponent<TransformComponent>()) {
			auto& comp = entity.GetComponent<TransformComponent>();
			rotation = comp.rotation;
		}
	}

	void SetRotation_Transform(UUID uuid, vec3& rotation) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		if (entity.HasComponent<TransformComponent>()) {
			auto& comp = entity.GetComponent<TransformComponent>();
			comp.dirty = true;
			comp.rotation = rotation;
		}
	}

	void GetScale_Transform(UUID uuid, vec3& scale) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		if (entity.HasComponent<TransformComponent>()) {
			auto& comp = entity.GetComponent<TransformComponent>();
			scale = comp.scale;
		}
	}

	void SetScale_Transform(UUID uuid, vec3& scale) {
		auto entity = Scripting::GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");

		if (entity.HasComponent<TransformComponent>()) {
			auto& comp = entity.GetComponent<TransformComponent>();
			comp.dirty = true;
			comp.scale = scale;
		}
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

	bool GetKeyDown(KeyCode key) {
		return Input::GetKeyDown(key);
	}

	bool GetKeyUp(KeyCode key) {
		return Input::GetKeyUp(key);
	}

	bool GetKey(KeyCode key) {
		return Input::GetKey(key);
	}
}