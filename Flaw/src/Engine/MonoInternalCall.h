#pragma once

#include "Core.h"
#include "Scripting/MonoScripting.h"
#include "Entity.h"
#include "Math/Math.h"
#include "Input/Input.h"
#include "Components.h"
#include "Utils/UUID.h"

namespace flaw {
	void LogInfo(MonoString* text);
		
	void DestroyEntity(UUID uuid);
	uint64_t FindEntityByName(MonoString* name);

	uint64_t CreateEntity_Prefab(AssetHandle prefab);
	uint64_t CreateEntityWithTransform_Prefab(AssetHandle prefab, vec3& position, vec3& rotation, vec3& scale);
	
	void GetPosition_Transform(UUID uuid, vec3& position);
	void SetPosition_Transform(UUID uuid, vec3& position);
	void GetRotation_Transform(UUID uuid, vec3& rotation);
	void SetRotation_Transform(UUID uuid, vec3& rotation);
	void GetScale_Transform(UUID uuid, vec3& scale);
	void SetScale_Transform(UUID uuid, vec3& scale);
	void GetForward_Transform(UUID uuid, vec3& forward);
	
	void GetBodyType_RigidBody2D(UUID uuid, int32_t& bodyType);
	void SetBodyType_RigidBody2D(UUID uuid, int32_t bodyType);
	void GetLinearVelocity_RigidBody2D(UUID uuid, vec2& velocity);

	void ScreenToWorld_Camera(UUID uuid, vec2& screenPos, vec3& worldPos);

	bool GetKeyDown_Input(KeyCode key);
	bool GetKeyUp_Input(KeyCode key);
	bool GetKey_Input(KeyCode key);
	void GetMousePosition_Input(float& x, float& y);

	bool Raycast_Physics(const Ray& ray, RayHit& hit);
}
