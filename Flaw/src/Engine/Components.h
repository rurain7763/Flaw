#pragma once

#include "Core.h"
#include "Utils/UUID.h"
#include "Math/Math.h"
#include "Graphics/Texture.h"
#include "Scriptable.h"
#include "Scripting.h"
#include "Font/Font.h"
#include "Asset.h"
#include "Sound/SoundChannel.h"

namespace flaw {
	struct EntityComponent {
		UUID uuid;
		std::string name = "Entity";

		EntityComponent() = default;
		EntityComponent(const char* name) : name(name) {}
		EntityComponent(const EntityComponent& other) = default;
	};

	struct TransformComponent {
		vec3 position = vec3(0.0f);
		vec3 rotation = vec3(0.0f);
		vec3 scale = vec3(1.0f);

		bool dirty = true;
		mat4 worldTransform = mat4(1.0f);

		TransformComponent() = default;
		TransformComponent(const vec3& position, const vec3& rotation, const vec3& scale) : position(position), rotation(rotation), scale(scale) {}
		TransformComponent(const TransformComponent& other) = default;

		inline vec3 GetWorldPosition() const {
			return ExtractPosition(worldTransform);
		}

		inline vec3 GetWorldFront() {
			return normalize(mat3(worldTransform) * Forward);
		}

		inline vec3 GetWorldRight() {
			return normalize(cross(Up, GetWorldFront()));
		}

		inline vec3 GetWorldUp() {
			return normalize(cross(GetWorldFront(), GetWorldRight()));
		}
	};

	struct SpriteRendererComponent {
		AssetHandle texture;
		vec4 color = vec4(1.0f);

		SpriteRendererComponent() = default;
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
		enum class BodyType : int32_t {
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

	struct TextComponent {
		std::wstring text;
		AssetHandle font;
		vec4 color = vec4(1.0f);

		TextComponent() = default;
		TextComponent(const TextComponent& other) = default;
	};

	struct SoundListenerComponent {
		vec3 velocity = vec3(0.0f);

		SoundListenerComponent() = default;
		SoundListenerComponent(const SoundListenerComponent& other) = default;
	};

	struct SoundSourceComponent {
		enum class Signal {
			None,
			Play,
			Stop,
			Resume
		};

		AssetHandle sound;

		bool loop = false;
		bool autoPlay = false;
		float volume = 1.0f;

		Ref<SoundChannel> channel;
		Signal signal = Signal::None;

		SoundSourceComponent() = default;
		SoundSourceComponent(const SoundSourceComponent& other) = default;

		SoundSourceComponent& operator=(const SoundSourceComponent& other) {
			sound = other.sound;
			loop = other.loop;
			autoPlay = other.autoPlay;
			volume = other.volume;

			return *this;
		}
	};

	struct ParticleComponent {
		enum ModuleType {
			Emission = 0x1,
			Shape = 0x2,
			RandomSpeed = 0x4,
			RandomColor = 0x8,
			RandomSize = 0x10,
			ColorOverLifetime = 0x20,
			SizeOverLifetime = 0x40,
			Noise = 0x80,
			Renderer = 0x100,
		};

		enum class SpaceType {
			Local,
			World
		};

		int32_t maxParticles = 1024;
		SpaceType spaceType = SpaceType::Local;
		float startSpeed = 1.0f;
		float startLifeTime = 1.0f;
		vec4 startColor = vec4(1.0f);
		vec3 startSize = vec3(1.0f);

		uint32_t modules = 0;

		float timer = 0.0f;

		ParticleComponent() = default;
		ParticleComponent(const ParticleComponent& other) = default;

		ParticleComponent& operator=(const ParticleComponent& other) {
			maxParticles = other.maxParticles;
			spaceType = other.spaceType;
			startSpeed = other.startSpeed;
			startLifeTime = other.startLifeTime;
			startColor = other.startColor;
			startSize = other.startSize;
			modules = other.modules;

			return *this;
		}
	};

	// Mesh
	struct MeshFilterComponent {
		AssetHandle mesh;

		MeshFilterComponent() = default;
		MeshFilterComponent(const MeshFilterComponent& other) = default;
	};

	struct MeshRendererComponent {
		AssetHandle material;

		MeshRendererComponent() = default;
		MeshRendererComponent(const MeshRendererComponent& other) = default;
	};

	// Light
	struct SkyLightComponent {
		vec3 color = vec3(1.0f);
		float intensity = 1.0f;

		SkyLightComponent() = default;
		SkyLightComponent(const SkyLightComponent& other) = default;
	};

	struct DirectionalLightComponent {
		vec3 color = vec3(1.0f);
		float intensity = 1.0f;

		DirectionalLightComponent() = default;
		DirectionalLightComponent(const DirectionalLightComponent& other) = default;
	};

	struct PointLightComponent {
		vec3 color = vec3(1.0f);
		float intensity = 1.0f;
		float range = 1.0f;

		PointLightComponent() = default;
		PointLightComponent(const PointLightComponent& other) = default;
	};

	struct SpotLightComponent {
		vec3 color = vec3(1.0f);
		float intensity = 1.0f;
		float inner = 0.5f;
		float outer = 1.0f;
		float range = 1.0f;

		SpotLightComponent() = default;
		SpotLightComponent(const SpotLightComponent& other) = default;
	};
}