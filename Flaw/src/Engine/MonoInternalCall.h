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
	
	float GetDeltaTime();
	float GetTimeSinceStart();
	
	void DestroyEntity(UUID uuid);
	uint64_t FindEntityByName(MonoString* name);

	uint64_t CreateEntity_Prefab(AssetHandle prefab);
	
	void GetPosition_Transform(UUID uuid, vec3& position);
	void SetPosition_Transform(UUID uuid, vec3& position);
	void GetRotation_Transform(UUID uuid, vec3& rotation);
	void SetRotation_Transform(UUID uuid, vec3& rotation);
	void GetScale_Transform(UUID uuid, vec3& scale);
	void SetScale_Transform(UUID uuid, vec3& scale);
	
	void GetBodyType_RigidBody2D(UUID uuid, int32_t& bodyType);
	void SetBodyType_RigidBody2D(UUID uuid, int32_t bodyType);
	void GetLinearVelocity_RigidBody2D(UUID uuid, vec2& velocity);

	bool GetKeyDown(KeyCode key);
	bool GetKeyUp(KeyCode key);
	bool GetKey(KeyCode key);
}
