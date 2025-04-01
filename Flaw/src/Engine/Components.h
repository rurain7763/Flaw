#pragma once

#include "Core.h"
#include "UUID.h"
#include "Math/Math.h"
#include "Graphics/Texture.h"
#include "Scriptable.h"
#include "Scripting.h"

namespace flaw {
	struct EntityComponent {
		UUID uuid;
		std::string name = "Entity";

		EntityComponent() = default;
		EntityComponent(const char* name) : name(name) {}
		EntityComponent(const EntityComponent& other) = default;

		EntityComponent& operator=(const EntityComponent& other) {
			name = other.name;
			return *this;
		}
	};

	struct TransformComponent {
		vec3 position;
		vec3 rotation;
		vec3 scale;

		TransformComponent() : position(vec3(0.0f)), rotation(vec3(0.0f)), scale(vec3(1.0f)) {}
		TransformComponent(const vec3& position, const vec3& rotation, const vec3& scale) : position(position), rotation(rotation), scale(scale) {}
		TransformComponent(const TransformComponent& other) = default;

		mat4 GetTransform() const {
			return ModelMatrix(position, rotation, scale);
		}
	};

	struct SpriteRendererComponent {
		Ref<Texture> texture;
		vec4 color;

		SpriteRendererComponent() : color(vec4(1.0f)) {}
		SpriteRendererComponent(const vec4& color) : color(color) {}
		SpriteRendererComponent(const Ref<Texture>& texture, const vec4& color) : texture(texture), color(color) {}
		SpriteRendererComponent(const SpriteRendererComponent& other) = default;
	};

	struct CameraComponent {
		bool perspective = false;

		// perspective attributes
		float fov;

		// orthographic attributes
		float orthoSize; // half of height

		float aspectRatio;
		float nearClip;
		float farClip;

		uint32_t depth = 0;

		CameraComponent() : perspective(false), fov(45.0f), orthoSize(1.0f), aspectRatio(16.0f / 9.0f), nearClip(0.1f), farClip(100.0f) {}

		CameraComponent(bool perspective, float fov, float aspectRatio, float nearClip, float farClip, uint32_t depth = 0)
			: perspective(perspective), fov(fov), aspectRatio(aspectRatio), nearClip(nearClip), farClip(farClip), depth(depth) {
		}

		CameraComponent(float orthoSize, float aspectRatio, float nearClip, float farClip, uint32_t depth = 0)
			: perspective(false), orthoSize(orthoSize), aspectRatio(aspectRatio), nearClip(nearClip), farClip(farClip), depth(depth) {
		}

		CameraComponent(const CameraComponent& other) = default;

		mat4 GetProjectionMatrix() const {
			if (perspective) {
				return Perspective(fov, aspectRatio, nearClip, farClip);
			}
			else {
				const float height = orthoSize;
				const float width = height * aspectRatio;
				return Orthographic(-width, width, -height, height, nearClip, farClip);
			}
		}
	};

	struct NativeScriptComponent {
		Scope<Scriptable> instance;
		std::function<Scope<Scriptable>()> instantiateFunc;

		NativeScriptComponent() = default;
		NativeScriptComponent(const NativeScriptComponent& other) {
			instantiateFunc = other.instantiateFunc;
		}

		virtual ~NativeScriptComponent() = default;

		NativeScriptComponent& operator=(const NativeScriptComponent& other) {
			instantiateFunc = other.instantiateFunc;
			return *this;
		}

		template <typename T, typename... Args>
		void Bind(Args&&... args) {
			instantiateFunc = [args...]() {
				return CreateScope<T>(args...);
			};
		}
	};

	struct MonoScriptComponent {
		std::string name;

		MonoScriptComponent() = default;
		MonoScriptComponent(const char* name) : name(name) {}
		MonoScriptComponent(const MonoScriptComponent& other) = default;

		MonoScriptComponent& operator=(const MonoScriptComponent& other) {
			name = other.name;
			return *this;
		}
	};

	// 2D physics
	struct Rigidbody2DComponent {
		enum class BodyType {
			Static,
			Dynamic,
			Kinematic
		};

		BodyType bodyType = BodyType::Static;
		bool fixedRotation = false;

		float density = 1.0f;
		float friction = 0.2f;
		float restitution = 0.0f;
		float restitutionThreshold = 0.5f;

		vec2 linearVelocity = vec2(0.0f);

		void* runtimeBody = nullptr;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent& other) {
			bodyType = other.bodyType;
			fixedRotation = other.fixedRotation;
			density = other.density;
			friction = other.friction;
			restitution = other.restitution;
			restitutionThreshold = other.restitutionThreshold;
		}

		Rigidbody2DComponent& operator=(const Rigidbody2DComponent& other) {
			bodyType = other.bodyType;
			fixedRotation = other.fixedRotation;
			density = other.density;
			friction = other.friction;
			restitution = other.restitution;
			restitutionThreshold = other.restitutionThreshold;
			return *this;
		}
	};

	struct BoxCollider2DComponent {
		vec2 offset = vec2(0.0f);
		vec2 size = vec2(0.5);

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent& other) = default;
	};

	struct CircleCollider2DComponent {
		vec2 offset = vec2(0.0f);
		float radius = 0.5f;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent& other) = default;
	};
}