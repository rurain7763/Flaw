#pragma once

#include "Core.h"
#include "Scripting.h"
#include "Utils/UUID.h"
#include "Entity.h"

namespace flaw {
	class Application;
	class Scene;

	struct MonoScriptInstance {
		MonoScriptObject scriptObject;
		MonoMethod* createMethod;
		MonoMethod* startMethod;
		MonoMethod* updateMethod;
		MonoMethod* destroyMethod;
	};

	class MonoScriptSystem {
	public:
		MonoScriptSystem(Application& app, Scene& scene);
		~MonoScriptSystem();

		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		void Start();
		void Update();
		void End();

		bool CreateMonoScriptInstance(const UUID& uuid, const char* name);
		void DestroyMonoScriptInstance(const UUID& uuid);
		bool IsMonoScriptInstanceExists(const UUID& uuid) const;
		MonoScriptInstance& GetMonoScriptInstance(const UUID& uuid);

		bool HasComponent(const UUID& uuid, const MonoScriptClass& compClass) const;

		void CloneMonoScriptInstances(const std::unordered_map<UUID, MonoScriptInstance>& instances);

		Scene& GetScene() const { return _scene; }
		float GetTimeSinceStart() const { return _timeSinceStart; }
		const std::unordered_map<UUID, MonoScriptInstance>& GetMonoInstances() const { return _monoInstances; }

	private:
		friend class Scripting;

		Application& _app;
		Scene& _scene;

		float _timeSinceStart = 0.f;

		std::unordered_map<UUID, MonoScriptInstance> _monoInstances;
	};
}