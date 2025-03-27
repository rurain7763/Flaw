#pragma once

#include "Core.h"
#include "Application.h"
#include "ECS/ECS.h"
#include "Log/Log.h"
#include "Graphics/GraphicsContext.h"
#include "Math/Math.h"

class b2World;

namespace flaw {
	class Entity;

	class Scene {
	public:
		Scene(Application& app);
		~Scene();

		Entity CreateEntity(const char* name = "Entity");
		void DestroyEntity(Entity entity);
		Entity CloneEntity(const Entity& srcEntt);
		Entity FindEntityByName(const char* name);

		void OnStart();
		void OnUpdate();
		void OnEnd();

		void ToFile(const char* filepath);
		void FromFile(const char* filepath);

		Ref<Scene> Clone();

		entt::registry& GetRegistry() { return _registry; }

	private:
		friend class Entity;

		Application& _app;

		entt::registry _registry;
		Scope<b2World> _physics2DWorld;
	};
}


