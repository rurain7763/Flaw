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

	class Scene {
	public:
		Scene(Application& app);
		~Scene();

		Entity CreateEntity(const char* name = "Entity");
		Entity CreateEntityByUUID(const UUID& uuid, const char* name = "Entity");
		void DestroyEntity(Entity entity);
		Entity CloneEntity(const Entity& srcEntt, bool sameUUID = false);

		Entity FindEntityByName(const char* name);
		Entity FindEntityByUUID(const UUID& uuid);

		void OnStart();
		void OnUpdate();
		void OnEnd();

		void UpdateSound();
		void UpdateScript();
		void UpdatePhysics2D();
		void UpdateTransform();
		void UpdateRender();

		void ToFile(const char* filepath);
		void FromFile(const char* filepath);

		Ref<Scene> Clone();

		entt::registry& GetRegistry() { return _registry; }
		ParticleSystem& GetParticleSystem() { return *_particleSystem; }

	private:
		void DestroyEntityRecursive(Entity entity);

	private:
		friend class Entity;

		Application& _app;

		entt::registry _registry;
		Scope<b2World> _physics2DWorld;
		Scope<ParticleSystem> _particleSystem;

		std::unordered_map<UUID, entt::entity> _entityMap; // uuid -> entity

		std::unordered_map<UUID, UUID> _parentMap; // child -> parent
		std::unordered_map<UUID, std::unordered_set<UUID>> _childMap; // parent -> children
	};
}


