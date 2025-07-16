#pragma once

#include "Core.h"
#include "Application.h"
#include "ECS/ECS.h"
#include "Log/Log.h"
#include "Graphics/GraphicsContext.h"
#include "Math/Math.h"
#include "Utils/UUID.h"

class b2World;

namespace flaw {
	class Entity;
	class ParticleSystem;
	class RenderSystem;
	class SkyBoxSystem;
	class LandscapeSystem;
	class ShadowSystem;
	class AnimationSystem;
	class MonoScriptSystem;
	class PhysicsSystem;
	class SkeletalSystem;
	class TransformSystem;
	class UISystem;

	class Scene {
	public:
		Scene(Application& app);
		~Scene();

		Entity CreateEntity(const char* name = "Entity");
		Entity CreateEntity(const vec3& position, const vec3& rotation = vec3(0.f), const vec3& scale = vec3(1.f), const char* name = "Entity");
		Entity CreateEntityByUUID(const UUID& uuid, const char* name = "Entity");
		void DestroyEntity(Entity entity);
		void DestroyEntityByUUID(const UUID& uuid);
		Entity CloneEntity(const Entity& srcEntt, bool sameUUID = false);

		Entity FindEntityByName(const char* name);
		Entity FindEntityByUUID(const UUID& uuid);

		void OnStart();
		void OnUpdate();
		void OnEnd();

		void UpdateSound();
		void UpdateScript();
		void UpdatePhysics2D();

		void ToFile(const char* filepath);
		void FromFile(const char* filepath);

		Ref<Scene> Clone();

		entt::registry& GetRegistry() { return _registry; }
		ParticleSystem& GetParticleSystem() { return *_particleSystem; }
		RenderSystem& GetRenderSystem() { return *_renderSystem; }
		SkyBoxSystem& GetSkyBoxSystem() { return *_skyBoxSystem; }
		LandscapeSystem& GetLandscapeSystem() { return *_landscapeSystem; }
		ShadowSystem& GetShadowSystem() { return *_shadowSystem; }
		AnimationSystem& GetAnimationSystem() { return *_animationSystem; }
		MonoScriptSystem& GetMonoScriptSystem() { return *_monoScriptSystem; }
		PhysicsSystem& GetPhysicsSystem() { return *_physicsSystem; }
		SkeletalSystem& GetSkeletalSystem() { return *_skeletalSystem; }
		TransformSystem& GetTransformSystem() { return *_transformSystem; }
		UISystem& GetUISystem() { return *_uiSystem; }

	private:
		void DestroyEntityRecursive(Entity entity);

	private:
		friend class Entity;

		Application& _app;

		entt::registry _registry;
		Scope<b2World> _physics2DWorld;
		Scope<ParticleSystem> _particleSystem;
		Scope<RenderSystem> _renderSystem;
		Scope<SkyBoxSystem> _skyBoxSystem;
		Scope<LandscapeSystem> _landscapeSystem;
		Scope<ShadowSystem> _shadowSystem;
		Scope<AnimationSystem> _animationSystem;
		Scope<MonoScriptSystem> _monoScriptSystem;
		Scope<PhysicsSystem> _physicsSystem;
		Scope<SkeletalSystem> _skeletalSystem;
		Scope<TransformSystem> _transformSystem;
		Scope<UISystem> _uiSystem;

		std::unordered_map<UUID, entt::entity> _entityMap; // uuid -> entity

		std::unordered_map<UUID, UUID> _parentMap; // child -> parent
		std::unordered_map<UUID, std::unordered_set<UUID>> _childMap; // parent -> children
	};
}


